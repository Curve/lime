#pragma once
#include <string>
#include <filesystem>

namespace lime
{
    class console
    {
        struct impl;

      public:
        static void restore();
        static void alloc_console(const std::string &name);
        static void redirect_to_file(const std::filesystem::path &);
    };
} // namespace lime