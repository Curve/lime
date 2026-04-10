#pragma once

#include "address.hpp"

namespace lime
{
    template <typename T>
    const T *address::as() const
    {
        return reinterpret_cast<const T *>(ptr());
    }

    template <typename T>
    std::optional<T *> address::as_mut() const
    {
        const auto rtn = mut_ptr();

        if (!rtn.has_value())
        {
            return std::nullopt;
        }

        return reinterpret_cast<T *>(*rtn);
    }

    template <typename T>
    bool address::write(const T &value)
    {
        return write({reinterpret_cast<const std::uint8_t *>(std::addressof(value)), sizeof(T)});
    }

    template <typename T>
    [[nodiscard]] T address::read()
    {
        return *as<T>();
    }
} // namespace lime
