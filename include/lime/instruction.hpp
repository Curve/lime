#pragma once

#include <vector>
#include <memory>

#include <variant>
#include <cstdint>
#include <optional>

namespace lime
{
    struct imm
    {
        using amount_t = std::variant<std::uint64_t, std::int64_t>;

      public:
        bool relative;
        amount_t amount;

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

    class instruction
    {
        struct impl;

      private:
        std::unique_ptr<impl> m_impl;

      private:
        instruction();

      public:
        ~instruction();

      public:
        instruction(const instruction &);
        instruction(instruction &&) noexcept;

      public:
        instruction &operator=(instruction &&) noexcept;

      public:
        [[nodiscard]] std::uintptr_t addr() const;

      public:
        [[nodiscard]] std::size_t size() const;
        [[nodiscard]] std::size_t mnemonic() const;

      public:
        [[nodiscard]] bool relative() const;
        [[nodiscard]] bool branching() const;

      public:
        [[nodiscard]] disp displacement() const;
        [[nodiscard]] std::vector<imm> immediates() const;

      public:
        [[nodiscard]] std::optional<instruction> prev() const;
        [[nodiscard]] std::optional<instruction> next() const;

      public:
        [[nodiscard]] std::optional<instruction> follow() const;
        [[nodiscard]] std::optional<instruction> next(std::size_t mnemonic) const;

      public:
        [[nodiscard]] std::optional<std::uintptr_t> absolute() const;
        [[nodiscard]] std::optional<std::uintptr_t> absolute(std::uintptr_t rip) const;

      public:
        [[nodiscard]] std::optional<instruction> operator-(std::size_t) const;
        [[nodiscard]] std::optional<instruction> operator+(std::size_t) const;

      public:
        [[nodiscard]] std::strong_ordering operator<=>(const instruction &) const;

      public:
        [[nodiscard]] static instruction unsafe(std::uintptr_t address);
        [[nodiscard]] static std::optional<instruction> at(std::uintptr_t address);
    };
} // namespace lime
