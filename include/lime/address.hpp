#pragma once
#include <memory>
#include <vector>
#include <cstdint>
#include <optional>

namespace lime
{
    class address
    {
        struct impl;

      private:
        std::unique_ptr<impl> m_impl;

      private:
        address();

      public:
        ~address();

      public:
        address(const address &);
        address(address &&) noexcept;

      public:
        bool write(const void *data, std::size_t size);
        [[nodiscard]] std::vector<std::uint8_t> copy(std::size_t size);

      public:
        template <typename T>
        bool write(const T &value);

      public:
        template <typename T>
        [[nodiscard]] T read();

      public:
        [[nodiscard]] void *ptr() const;
        [[nodiscard]] std::uintptr_t addr() const;

      public:
        [[nodiscard]] std::optional<address> operator-(std::size_t) const;
        [[nodiscard]] std::optional<address> operator+(std::size_t) const;

      public:
        [[nodiscard]] operator void *() const;
        [[nodiscard]] operator std::uintptr_t() const;

      public:
        [[nodiscard]] std::strong_ordering operator<=>(const address &) const;

      public:
        [[nodiscard]] static address unsafe(std::uintptr_t address);
        [[nodiscard]] static std::optional<address> at(std::uintptr_t address);
    };
} // namespace lime

#include "address.inl"
