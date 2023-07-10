#include "address.hpp"
#include "instruction.hpp"
#include "page.hpp"

#include <cstring>

namespace lime
{
    struct address::impl
    {
        std::uintptr_t address;
    };

    address::address() : m_impl(std::make_unique<impl>()) {}

    address::~address() = default;

    address::address(const address &other) : m_impl(std::make_unique<impl>())
    {
        *m_impl = *other.m_impl;
    }

    address::address(address &&other) noexcept : m_impl(std::move(other.m_impl)) {}

    bool address::write(const void *data, std::size_t size)
    {
        auto page = lime::page::at(m_impl->address);

        if (!page || !(page->prot() & protection::write))
        {
            return false;
        }

        auto *dest = reinterpret_cast<void *>(m_impl->address);
        std::memcpy(dest, data, size);

        return true;
    }

    std::vector<std::uint8_t> address::copy(std::size_t size)
    {
        auto *src = reinterpret_cast<std::uint8_t *>(m_impl->address);

        std::vector<std::uint8_t> rtn;
        std::copy(src, src + size, std::back_inserter(rtn));

        return rtn;
    }

    void *address::ptr() const
    {
        return reinterpret_cast<void *>(m_impl->address);
    }

    std::uintptr_t address::addr() const
    {
        return m_impl->address;
    }

    std::optional<address> address::operator+(const std::size_t amount) const
    {
        return at(m_impl->address + amount);
    }

    std::optional<address> address::operator-(const std::size_t amount) const
    {
        return at(m_impl->address - amount);
    }

    address::operator void *() const
    {
        return ptr();
    }

    address::operator std::uintptr_t() const
    {
        return addr();
    }

    std::strong_ordering address::operator<=>(const address &other) const
    {
        auto address = static_cast<std::uintptr_t>(other);

        if (address > m_impl->address)
        {
            return std::strong_ordering::less;
        }

        if (address < m_impl->address)
        {
            return std::strong_ordering::greater;
        }

        return std::strong_ordering::equal;
    }

    address address::unsafe(std::uintptr_t address)
    {
        lime::address rtn;
        rtn.m_impl->address = address;

        return rtn;
    }

    std::optional<address> address::at(std::uintptr_t address)
    {
        auto page = page::at(address);

        if (!page || !(page->prot() & protection::read))
        {
            return std::nullopt;
        }

        return unsafe(address);
    }
} // namespace lime