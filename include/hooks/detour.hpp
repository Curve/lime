#pragma once
#include <cstdint>
#include <memory>
#include <page.hpp>

namespace lime
{
    class detour
    {
        template <typename type_t> //
        using func_t = std::conditional_t<std::is_pointer_v<type_t>, type_t, std::add_pointer_t<type_t>>;

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
        template <typename type_t> func_t<type_t> get_original() const;

      public:
        template <typename target_t, typename replacement_t>
        static std::unique_ptr<detour> create(const target_t &target, const replacement_t &replacement);
        static std::unique_ptr<detour> create(const std::uintptr_t &target, const std::uintptr_t &replacement);
    };
} // namespace lime

#include "detour.inl"