#pragma once
#include <string>
#include <memory>
#include <vector>
#include <cstdint>
#include <optional>

namespace lime
{
    struct symbol
    {
        std::string name;
        std::uintptr_t address;
    };

    class module
    {
        struct impl;

      private:
        std::unique_ptr<impl> m_impl;

      private:
        module();

      public:
        ~module();

      public:
        module(const module &);
        module(module &&) noexcept;

      public:
        [[nodiscard]] std::string name() const;

      public:
        [[nodiscard]] std::size_t size() const;

      public:
        [[nodiscard]] std::uintptr_t end() const;
        [[nodiscard]] std::uintptr_t start() const;

      public:
        [[nodiscard]] std::vector<lime::symbol> symbols() const;

      public:
        [[nodiscard]] std::uintptr_t symbol(const std::string &name) const;
        [[nodiscard]] std::optional<std::uintptr_t> find_symbol(const std::string &name) const;

      public:
        [[nodiscard]] static std::vector<module> modules();

      public:
        [[nodiscard]] static std::optional<module> get(const std::string &name);
        [[nodiscard]] static std::optional<module> find(const std::string &name);
    };
} // namespace lime