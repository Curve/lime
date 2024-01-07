#pragma once

#include "address.hpp"

namespace lime
{
    template <typename T>
    bool address::write(const T &value)
    {
        return write(reinterpret_cast<const void *>(&value), sizeof(T));
    }

    template <typename T>
    [[nodiscard]] T address::read()
    {
        return *reinterpret_cast<T *>(ptr());
    }
} // namespace lime
