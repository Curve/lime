#pragma once

#include "../page.hpp"
#include "../module.hpp"

#include <memory>
#include <cstdint>
#include <optional>

#include <string>
#include <string_view>

namespace lime
{
    enum class find_policy
    {
        all,
        one,
    };

    class signature
    {
        struct impl;

      private:
        std::unique_ptr<impl> m_impl;

      private:
        signature();

      public:
        ~signature();

      public:
        signature(const signature &);
        signature(signature &&) noexcept;

      public:
        [[nodiscard]] std::optional<std::uintptr_t> find() const;
        [[nodiscard]] std::optional<std::uintptr_t> find(const page &) const;
        [[nodiscard]] std::optional<std::uintptr_t> find(const module &) const;

      public:
        [[nodiscard]] std::vector<std::uintptr_t> find_all() const;
        [[nodiscard]] std::vector<std::uintptr_t> find_all(const page &) const;
        [[nodiscard]] std::vector<std::uintptr_t> find_all(const module &) const;

      public:
        static signature from(std::string_view ida_pattern, protection required = protection::read);
        static signature from(const char *pattern, std::string mask, protection required = protection::read);
    };
} // namespace lime
