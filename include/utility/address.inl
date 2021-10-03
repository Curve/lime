#pragma once

namespace lime
{
    template <typename T> T address::get_as() const
    {
        return *reinterpret_cast<T *>(get());
    }

    template <typename T> std::optional<T> address::get_as_safe() const
    {
        const auto safe_address = get_safe();
        if (safe_address)
        {
            return *reinterpret_cast<T *>(*safe_address);
        }

        return std::nullopt;
    }
} // namespace lime