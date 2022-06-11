#pragma once
#include "detour.hpp"
#include <type_traits>

namespace lime
{
    template <typename type_t> detour::func_t<type_t> detour::get_original() const
    {
        return reinterpret_cast<func_t<type_t>>(m_original_func);
    }

    template <typename target_t, typename replacement_t> std::unique_ptr<detour> detour::create(const target_t &target, const replacement_t &replacement, detour_status &status)
    {
        return detour::create(reinterpret_cast<std::uintptr_t>(target), reinterpret_cast<std::uintptr_t>(replacement), status);
    }

    template <typename target_t, typename replacement_t> std::unique_ptr<detour> detour::create(const target_t &target, const replacement_t &replacement)
    {
        return detour::create(reinterpret_cast<std::uintptr_t>(target), reinterpret_cast<std::uintptr_t>(replacement));
    }
} // namespace lime