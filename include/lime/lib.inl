#pragma once

#include "lib.hpp"

namespace lime
{
    template <typename T>
    std::optional<std::uintptr_t> lib::operator[](T &&name) const
    {
        return symbol(std::forward<T>(name));
    }

    template <typename T, typename U>
    std::optional<U> lib::operator[](T &&name, std::type_identity<U>) const
    {
        return symbol(std::forward<T>(name)).transform([](auto &&value) { return reinterpret_cast<U>(value); });
    }
} // namespace lime
