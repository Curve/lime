#include "constants/protection.hpp"
#include "utility/address.hpp"
#include "disasm/disasm.hpp"
#include "page.hpp"

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

        if (page->get_protection() != prot::none && page->get_protection() != prot::execute)
        {
            return m_address;
        }

        return std::nullopt;
    }

    std::optional<address> address::next() const
    {
        if (get_safe())
        {
            return {m_address + disasm::get_size(m_address).value_or(0)};
        }

        return std::nullopt;
    }

    std::optional<address> address::follow() const
    {
        if (get_safe())
        {
            const auto result = disasm::follow(m_address);

            if (result)
                return {*result};
        }

        return std::nullopt;
    }

    std::optional<std::uint32_t> address::get_mnemonic() const
    {
        if (get_safe())
        {
            return disasm::get_mnemonic(m_address);
        }

        return std::nullopt;
    }

    std::vector<std::uintptr_t> address::get_immediates() const
    {
        if (get_safe())
        {
            return disasm::get_immediates(m_address);
        }

        return {};
    }

    std::optional<address> address::read_until(const std::uint32_t &mnemonic) const
    {
        const auto page = page::get_page_at(m_address);

        if (!page)
            return std::nullopt;

        const auto remaining = page->get_end() - m_address;

        if (page->get_protection() != prot::none && page->get_protection() != prot::execute)
        {
            const auto result = disasm::read_until(mnemonic, m_address, remaining);

            if (!result)
                return std::nullopt;

            return {*result};
        }

        return std::nullopt;
    }

    address address::operator+(const std::size_t &offset) const
    {
        return {m_address + offset};
    }

    address address::operator-(const std::size_t &offset) const
    {
        return {m_address - offset};
    }
} // namespace lime