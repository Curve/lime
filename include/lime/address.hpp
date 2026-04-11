#pragma once

#include <cstdint>
#include <optional>

#include <span>
#include <vector>

namespace lime
{
    class instruction;

    class address
    {
        std::uintptr_t m_address;

      private:
        address(std::uintptr_t);

      public:
        [[nodiscard]] std::uintptr_t value() const;

      public:
        [[nodiscard]] const void *ptr() const;
        [[nodiscard]] std::optional<void *> mut_ptr() const;

      public:
        bool write(std::span<const std::uint8_t>) const; // NOLINT(*-nodiscard)
        [[nodiscard]] std::vector<std::uint8_t> copy(std::size_t size) const;

      public:
        template <typename T>
        [[nodiscard]] const T *as() const;

        template <typename T>
        [[nodiscard]] std::optional<T *> as_mut() const;

        template <typename T>
        bool write(const T &);

        template <typename T>
        [[nodiscard]] T read();

      public:
        [[nodiscard]] std::optional<address> operator-(std::size_t) const;
        [[nodiscard]] std::optional<address> operator+(std::size_t) const;

      public:
        [[nodiscard]] std::strong_ordering operator<=>(const address &) const = default;

      public:
        [[nodiscard]] static address unsafe(std::uintptr_t);
        [[nodiscard]] static std::optional<address> at(std::uintptr_t);
    };
} // namespace lime

#include "address.inl"
