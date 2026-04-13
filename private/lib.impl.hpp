#pragma once

#include "lib.hpp"

#include <regex>

namespace lime
{
    struct pattern::impl
    {
        std::string raw;
        std::regex regex;
    };
} // namespace lime
