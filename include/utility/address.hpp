#pragma once
#include <constants/mnemonics.hpp>
#include <cstdint>
#include <optional>

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

      public:
        std::uintptr_t get() const;
        std::optional<std::uintptr_t> get_safe() const;

        template <typename T> T get_as() const;
        template <typename T> std::optional<T> get_as_safe() const;

        std::optional<address> follow() const;
        std::optional<address> read_until(const std::uint8_t &mnemonic) const;
    };
} // namespace lime

#include "address.inl"