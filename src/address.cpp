#include "address.hpp"

#include "page.hpp"

#include <cstring>

namespace lime
{
    address::address(std::uintptr_t address) : m_address(address) {}

    std::uintptr_t address::value() const
    {
        return m_address;
    }

    const void *address::ptr() const
    {
        return reinterpret_cast<const void *>(m_address);
    }

    std::optional<void *> address::mut_ptr() const
    {
        if (const auto page = lime::page::at(m_address); !page || !page->can(protection::write))
        {
            return std::nullopt;
        }

        return reinterpret_cast<void *>(m_address);
    }

    bool address::write(std::span<const std::uint8_t> data) const
    {
        auto page = lime::page::at(m_address);

        if (!page.has_value())
        {
            return false;
        }

        const auto writable = page->can(protection::write);
        auto *const ptr     = reinterpret_cast<void *>(m_address);

        if (!writable && !page->allow(protection::write))
        {
            return false;
        }

        std::memcpy(ptr, data.data(), data.size());

        if (!writable)
        {
            page->restore();
        }

        return true;
    }

    std::vector<std::uint8_t> address::copy(std::size_t size) const
    {
        const auto *const src = reinterpret_cast<const std::uint8_t *>(m_address);
        return {src, src + size};
    }

    std::optional<address> address::operator+(const std::size_t amount) const
    {
        return at(m_address + amount);
    }

    std::optional<address> address::operator-(const std::size_t amount) const
    {
        return at(m_address - amount);
    }

    address address::unsafe(std::uintptr_t address)
    {
        return {address};
    }

    std::optional<address> address::at(std::uintptr_t address)
    {
        if (const auto page = lime::page::at(address); !page || !page->can(protection::read))
        {
            return std::nullopt;
        }

        return unsafe(address);
    }
} // namespace lime
