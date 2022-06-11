#pragma once
#include <memory>
#include <vector>
#include <cstdint>
#include <optional>

namespace lime
{
    class page
    {
        struct impl;

      private:
        page();

      public:
        page(const page &);

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