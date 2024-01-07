#pragma once

#include <vector>
#include <memory>

#include <cstdint>
#include <optional>

#include <flagpp/flags.hpp>

namespace lime
{
    enum class protection
    {
        none,
        read    = 1 << 0,
        write   = 1 << 1,
        execute = 1 << 2,
    };

    enum class alloc_policy
    {
        exact,
        nearby,
    };

    class page
    {
        struct impl;

      private:
        std::unique_ptr<impl> m_impl;

      private:
        page();

      public:
        ~page();

      public:
        page(const page &);
        page(page &&) noexcept;

      public:
        page &operator=(page &&) noexcept;

      public:
        [[nodiscard]] protection prot() const;

      public:
        [[nodiscard]] std::size_t size() const;

      public:
        [[nodiscard]] std::uintptr_t end() const;
        [[nodiscard]] std::uintptr_t start() const;

      public:
        bool restore();
        bool protect(protection prot);

      public:
        [[nodiscard]] static std::vector<page> pages();

      public:
        [[nodiscard]] static page unsafe(std::uintptr_t address);
        [[nodiscard]] static std::optional<page> at(std::uintptr_t address);

      public:
        [[nodiscard]] static std::shared_ptr<page> allocate(std::size_t size, protection prot);

      public:
        template <alloc_policy Policy = alloc_policy::nearby>
        [[nodiscard]] static std::shared_ptr<page> allocate(std::uintptr_t where, std::size_t size, protection prot);
    };

    template <>
    std::shared_ptr<page> page::allocate<alloc_policy::exact>(std::uintptr_t, std::size_t, protection);

    template <>
    std::shared_ptr<page> page::allocate<alloc_policy::nearby>(std::uintptr_t, std::size_t, protection);
} // namespace lime

template <>
constexpr inline bool flagpp::enabled<lime::protection> = true;
