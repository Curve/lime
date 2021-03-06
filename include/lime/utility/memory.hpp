#pragma once
#include <memory>
#include <cstdint>
#include <optional>

namespace lime
{
    bool write(const std::uintptr_t &address, const void *data, const std::size_t &size);
    template <class type_t> bool write(const std::uintptr_t &address, const type_t &value);

    std::unique_ptr<char[]> read(const std::uintptr_t &address, const std::size_t &size);
    template <class type_t> std::optional<type_t> read(const std::uintptr_t &address);

    bool free(const std::uintptr_t &address, const std::size_t &size);
    bool protect(const std::uintptr_t &address, const std::size_t &size, const std::uint8_t &protection);

    std::shared_ptr<std::uintptr_t> allocate(const std::size_t &size, const std::uint8_t &protection);
    bool allocate_at(const std::uintptr_t &address, const std::size_t &size, const std::uint8_t &protection);
    std::shared_ptr<std::uintptr_t> allocate_near(const std::uintptr_t &address, const std::size_t &size, const std::uint8_t &protection);
} // namespace lime

#include "memory.inl"