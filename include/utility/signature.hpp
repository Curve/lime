#pragma once
#include <cstdint>
#include <module.hpp>
#include <optional>
#include <page.hpp>
#include <string>
#include <vector>

namespace lime
{
    class signature
    {
        template <bool find_all> using cond_rtn = std::conditional_t<find_all, std::vector<std::uintptr_t>, std::optional<std::uintptr_t>>;

      public:
        signature(const std::string &ida_pattern);
        template <std::size_t size> signature(const char (&pattern)[size], const char *mask);

      private:
        std::string m_mask;
        std::string m_pattern;

      public:
        template <bool find_all> cond_rtn<find_all> find();
        template <bool find_all> cond_rtn<find_all> find_in(const page &);
        template <bool find_all> cond_rtn<find_all> find_in(const module &);
    };

    using sig = signature;
} // namespace lime

#include "signature.inl"