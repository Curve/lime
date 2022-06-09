#pragma once
#include "memory.hpp"

template <typename type_t> bool lime::write(const std::uintptr_t &address, const type_t &value)
{
    return (*reinterpret_cast<type_t *>(address) = value) == value;
}

template <typename type_t> std::optional<type_t> lime::read(const std::uintptr_t &address)
{
    return *reinterpret_cast<type_t *>(address);
}