#pragma once
#include <type_traits>

namespace lime
{
    template <typename type_t> detour::func_t<type_t> detour::get_original() const
    {
        return reinterpret_cast<func_t<type_t>>(m_original_func);
    }
} // namespace lime