#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace lime
{
    class page
    {
        struct impl;

      private:
        page();
        std::unique_ptr<impl> m_impl;

      public:
        ~page();
        page(page &&) noexcept;

      private:
        std::uint8_t m_prot;
        std::uintptr_t m_end;
        std::uintptr_t m_start;

      public:
        std::size_t get_end() const;
        std::uintptr_t get_start() const;
        std::uint8_t get_protection() const;

      public:
        static std::vector<page> get_pages();
        static std::optional<page> get_page_at(const std::uintptr_t &);
    };
} // namespace lime