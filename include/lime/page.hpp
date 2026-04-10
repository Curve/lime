#pragma once

#include <cstdint>
#include <memory>

#include <vector>
#include <optional>

#include <flagpp/flags.hpp>

namespace lime
{
    enum class protection : std::uint8_t
    {
        none    = 0,
        read    = 1 << 0,
        write   = 1 << 1,
        execute = 1 << 2,
    };

    class page
    {
        struct impl;

      public:
        enum class policy : std::uint8_t
        {
            exact,
            nearby,
        };

      private:
        std::unique_ptr<impl> m_impl;

      private:
        page(impl);

      public:
        page(const page &);
        page(page &&) noexcept;

      public:
        ~page();

      public:
        page &operator=(page) noexcept;
        friend void swap(page &, page &) noexcept;

      public:
        [[nodiscard]] std::uintptr_t start() const;
        [[nodiscard]] std::uintptr_t end() const;

      public:
        [[nodiscard]] std::size_t size() const;

      public:
        [[nodiscard]] protection prot() const;
        [[nodiscard]] bool can(protection) const;

      public:
        bool restore();
        bool allow(protection);
        bool protect(protection prot);

      public:
        template <policy = policy::nearby>
        [[nodiscard]] static std::optional<page> allocate(std::uintptr_t where, std::size_t size, protection prot);
        [[nodiscard]] static std::optional<page> allocate(std::size_t size, protection prot);

      public:
        [[nodiscard]] static std::vector<page> pages();
        [[nodiscard]] static std::optional<page> at(std::uintptr_t);
    };
} // namespace lime

template <>
constexpr inline bool flagpp::enabled<lime::protection> = true;
