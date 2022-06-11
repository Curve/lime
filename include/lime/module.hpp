#pragma once
#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace lime
{
    class module
    {
        struct impl;

      private:
        module();
        std::unique_ptr<impl> m_impl;

      public:
        ~module();
        module(module &&) noexcept;

      private:
        std::string m_name;
        std::size_t m_size;
        std::uintptr_t m_start;

      public:
        std::string get_name() const;
        std::size_t get_size() const;
        std::uintptr_t get_start() const;

        std::uintptr_t get_symbol(const std::string &) const;
        std::optional<std::uintptr_t> find_symbol(const std::string &) const;
        std::vector<std::pair<std::string, std::uintptr_t>> get_symbols() const;

      public:
        static std::vector<module> get_modules();
        static std::optional<module> get(const std::string &name);
        static std::optional<module> find(const std::string &name);
    };
} // namespace lime