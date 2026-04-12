#pragma once

#include <cstdint>

#include <memory>
#include <string_view>

#include <array>
#include <vector>

#include <variant>
#include <optional>

namespace lime
{
    class address;

    struct imm
    {
        bool relative;
        std::variant<std::uint64_t, std::int64_t> amount;

      public:
        std::size_t size;
        std::size_t offset;
    };

    struct disp
    {
        std::int64_t amount;

      public:
        std::size_t size;
        std::size_t offset;
    };

    using mnemonic = std::string_view;

    class instruction
    {
        struct impl;

      private:
        std::unique_ptr<impl> m_impl;

      private:
        instruction(impl);

      public:
        instruction(const instruction &);
        instruction(instruction &&) noexcept;

      public:
        ~instruction();

      public:
        instruction &operator=(instruction) noexcept;
        friend void swap(instruction &, instruction &) noexcept;

      public:
        [[nodiscard]] std::uintptr_t address() const;

      public:
        [[nodiscard]] std::size_t size() const;
        [[nodiscard]] lime::mnemonic mnemonic() const;

      public:
        [[nodiscard]] bool relative() const;
        [[nodiscard]] bool branching() const;

      public:
        [[nodiscard]] disp displacement() const;
        [[nodiscard]] std::array<imm, 2> immediates() const;

      public:
        [[nodiscard]] std::optional<instruction> next() const;
        [[nodiscard]] std::optional<instruction> next(lime::mnemonic) const;

      public:
        [[nodiscard]] std::vector<instruction> prev() const;
        [[nodiscard]] std::vector<instruction> prev(lime::mnemonic) const;

      public:
        [[nodiscard]] std::optional<std::uintptr_t> follow() const;
        [[nodiscard]] std::optional<std::uintptr_t> follow(std::uintptr_t rip) const;
        [[nodiscard]] std::optional<std::uintptr_t> follow(std::uintptr_t rip, std::size_t operand) const;

      public:
        [[nodiscard]] std::optional<instruction> operator-(std::size_t) const;
        [[nodiscard]] std::optional<instruction> operator+(std::size_t) const;

      public:
        [[nodiscard]] std::strong_ordering operator<=>(const instruction &) const;

      public:
        [[nodiscard]] static std::optional<instruction> at(std::uintptr_t);
        [[nodiscard]] static std::optional<instruction> at(const lime::address &);
    };
} // namespace lime
