#pragma once
#include "page.hpp"
#include "module.hpp"

#include <string>
#include <memory>
#include <cstdint>
#include <optional>

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
        static signature from(const std::string &ida_pattern, protection required = protection::none);
        static signature from(const char *pattern, std::string mask, protection required = protection::none);
    };
} // namespace lime