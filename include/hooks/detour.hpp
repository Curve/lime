#pragma once
#include <cstdint>
#include <memory>
#include <page.hpp>

namespace lime
{
    class detour
    {
      private:
        detour();

      public:
        ~detour();
        detour(detour &&) noexcept;

      private:
        std::uintptr_t m_target;
        std::uintptr_t m_replacement;
        std::uintptr_t m_original_func;
        std::unique_ptr<page> m_original_page;
        std::vector<std::uint8_t> m_original_code;
        std::shared_ptr<std::uintptr_t> m_trampoline;

      public:
        std::uintptr_t get_original() const;
        template <typename func_t> func_t get_original() const;

      public:
        static std::optional<detour> create(const std::uintptr_t &target, const std::uintptr_t &replacement);
    };
} // namespace lime