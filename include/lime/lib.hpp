#pragma once

#include <vector>
#include <memory>

#include <cstdint>

#include <regex>
#include <optional>

#include <string>
#include <string_view>

#include <functional>
#include <filesystem>

namespace lime
{
    namespace fs = std::filesystem;

    namespace literals
    {
        std::regex operator""_re(const char *, std::size_t);
    }

    struct symbol
    {
        std::string name;
        std::uintptr_t address;
    };

    class lib
    {
        struct impl;

      public:
        using lib_predicate = std::function<bool(const lib &)>;
        using sym_predicate = std::function<bool(const char *)>;

      private:
        std::unique_ptr<impl> m_impl;

      private:
        lib(impl);

      public:
        lib(const lib &);
        lib(lib &&) noexcept;

      public:
        ~lib();

      public:
        lib &operator=(lib) noexcept;
        friend void swap(lib &, lib &) noexcept;

      public:
        [[nodiscard]] std::uintptr_t start() const;
        [[nodiscard]] std::uintptr_t end() const;

      public:
        [[nodiscard]] std::string_view name() const;
        [[nodiscard]] std::size_t size() const;

      public:
        [[nodiscard]] std::vector<lime::symbol> symbols() const;

      public:
        [[nodiscard]] std::optional<std::uintptr_t> symbol(const char *) const;
        [[nodiscard]] std::optional<std::uintptr_t> symbol(const std::regex &) const;
        [[nodiscard]] std::optional<std::uintptr_t> symbol(const sym_predicate &) const;

      public:
        // TODO: member that returns function pointer => operator(), operator[]

      public:
        [[nodiscard]] static std::vector<lib> libraries();
        [[nodiscard]] static std::optional<lib> load(const fs::path &);

      public:
        [[nodiscard]] static std::optional<lib> find(const fs::path &);
        [[nodiscard]] static std::optional<lib> find(const std::regex &);
        [[nodiscard]] static std::optional<lib> find(const lib_predicate &);
    };
} // namespace lime
