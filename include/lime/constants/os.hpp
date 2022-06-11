#pragma once
#include <cstdint>

namespace lime
{
    enum class operating_system
    {
        Windows,
        Linux
    };

#if defined(_WIN32)
    constexpr operating_system os = operating_system::Windows;
#elif defined(__linux__)
    constexpr operating_system os = operating_system::Linux;
#endif
} // namespace lime