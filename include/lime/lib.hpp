#pragma once

#include "utils/convention.hpp"

#include <vector>
#include <memory>

#include <cstdint>
#include <type_traits>

#include <regex>
#include <optional>

#include <string>
#include <string_view>

#include <functional>
#include <filesystem>

namespace lime
{
    using utils::calling_convention;

    template <typename T, auto U = calling_convention::cc_generic>
    static constexpr auto id = std::type_identity<typename utils::convention_traits<T, U>::pointer>{};

    namespace fs = std::filesystem;

    struct pattern
    {
        std::regex regex;
        std::string raw;

      public:
        static pattern from(std::string_view);
    };

    namespace literals
    {
        /**
         * Example: "PlainText<Regex.*within[0-9]Angle\d+Brackets>"_re
         * You can also escape the angle brackets: "These\<Are\>Escaped"_re
         */
        pattern operator""_re(const char *, std::size_t);
    } // namespace literals

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
        [[nodiscard]] std::optional<std::uintptr_t> symbol(const pattern &) const;
        [[nodiscard]] std::optional<std::uintptr_t> symbol(const sym_predicate &) const;

      public:
        template <typename T>
        [[nodiscard]] std::optional<std::uintptr_t> operator[](T &&) const;

        template <typename T, typename U>
        [[nodiscard]] std::optional<U> operator[](T &&, std::type_identity<U>) const;

      public:
        [[nodiscard]] static std::vector<lib> libraries();
        [[nodiscard]] static std::optional<lib> load(const fs::path &);

      public:
        [[nodiscard]] static std::optional<lib> find(const pattern &);
        [[nodiscard]] static std::optional<lib> find(const fs::path &);
        [[nodiscard]] static std::optional<lib> find(const lib_predicate &);
    };
} // namespace lime

#include "lib.inl"
