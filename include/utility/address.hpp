#pragma once
#include <cstdint>
#include <optional>
#include <vector>

namespace lime
{
    class address
    {
      private:
        std::uintptr_t m_address;

      public:
        address();
        address(const std::uintptr_t &);

      public:
        bool operator>(const address &) const;
        bool operator<(const address &) const;
        bool operator==(const address &) const;

        address operator-(const std::size_t &) const;
        address operator+(const std::size_t &) const;

      public:
        std::uintptr_t get() const;
        std::optional<std::uintptr_t> get_safe() const;

        template <typename T> T get_as() const;
        template <typename T> std::optional<T> get_as_safe() const;

        std::optional<address> next() const;
        std::optional<address> follow() const;
        std::optional<std::uint32_t> get_mnemonic() const;
        std::vector<std::uintptr_t> get_immediates() const;
        std::optional<address> read_until(const std::uint32_t &mnemonic) const;
    };
} // namespace lime

#include "address.inl"