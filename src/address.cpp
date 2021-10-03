#include "disasm/disasm.hpp"
#include <constants/protection.hpp>
#include <page.hpp>
#include <utility/address.hpp>

namespace lime
{
    address::address() : m_address(0) {}
    address::address(const std::uintptr_t &address) : m_address(address) {}

    bool address::operator>(const address &other) const
    {
        return m_address > other.m_address;
    }
    bool address::operator<(const address &other) const
    {
        return m_address < other.m_address;
    }
    bool address::operator==(const address &other) const
    {
        return m_address == other.m_address;
    }

    std::uintptr_t address::get() const
    {
        return m_address;
    }
    std::optional<std::uintptr_t> address::get_safe() const
    {
        const auto page = page::get_page_at(m_address);

        if (!page)
            return std::nullopt;

        if (page->get_protection() == prot::read_only || page->get_protection() == prot::read_write ||
            page->get_protection() == prot::read_write_execute || page->get_protection() == prot::read_execute)
        {
            return m_address;
        }

        return std::nullopt;
    }

    std::optional<address> address::follow() const
    {
        if (get_safe())
        {
            const auto result = disasm::follow(m_address);

            if (result)
                return {result};
        }

        return std::nullopt;
    }

    std::optional<address> address::read_until(const std::uint8_t &mnemonic) const
    {
        const auto page = page::get_page_at(m_address);

        if (!page)
            return std::nullopt;

        const auto remaining = page->get_end() - m_address;

        if (page->get_protection() == prot::read_only || page->get_protection() == prot::read_write ||
            page->get_protection() == prot::read_write_execute || page->get_protection() == prot::read_execute)
        {
            const auto result = disasm::read_until(mnemonic, m_address, remaining);

            if (!result)
                return std::nullopt;

            return {*result};
        }

        return std::nullopt;
    }
} // namespace lime