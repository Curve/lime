#pragma once

#include <vector>
#include <memory>

#include <cstdint>
#include <optional>
#include <string_view>

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
        [[nodiscard]] std::string_view name() const;

      public:
        [[nodiscard]] std::size_t size() const;

      public:
        [[nodiscard]] std::uintptr_t end() const;
        [[nodiscard]] std::uintptr_t start() const;

      public:
        [[nodiscard]] std::vector<lime::symbol> symbols() const;

      public:
        [[nodiscard]] std::uintptr_t symbol(std::string_view name) const;
        [[nodiscard]] std::optional<std::uintptr_t> find_symbol(std::string_view name) const;

      public:
        [[nodiscard]] static std::vector<module> modules();

      public:
        [[nodiscard]] static std::optional<module> get(std::string_view name);
        [[nodiscard]] static std::optional<module> load(std::string_view name);

      public:
        [[nodiscard]] static std::optional<module> find(std::string_view name);
    };
} // namespace lime
