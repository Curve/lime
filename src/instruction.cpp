#include "instruction.hpp"
#include "constants.hpp"
#include "disasm.hpp"
#include "page.hpp"

namespace lime
{
    struct instruction::impl
    {
        std::uintptr_t address;
        std::shared_ptr<lime::page> page;
    };

    instruction::instruction() : m_impl(std::make_unique<impl>()) {}

    instruction::~instruction() = default;

    instruction::instruction(const instruction &other) : m_impl(std::make_unique<impl>())
    {
        *m_impl = *other.m_impl;
    }

    instruction::instruction(instruction &&other) noexcept : m_impl(std::move(other.m_impl)) {}

    instruction &instruction::operator=(instruction &&other) noexcept
    {
        m_impl = std::move(other.m_impl);
        return *this;
    }

    std::uintptr_t instruction::addr() const
    {
        return m_impl->address;
    }

    std::size_t instruction::size() const
    {
        return disasm::size(m_impl->address);
    }

    std::size_t instruction::mnemonic() const
    {
        return disasm::mnemonic(m_impl->address);
    }

    bool instruction::relative() const
    {
        return disasm::relative(m_impl->address);
    }

    bool instruction::branching() const
    {
        return disasm::branching(m_impl->address);
    }

    disp instruction::displacement() const
    {
        return disasm::displacement(m_impl->address);
    }

    std::vector<imm> instruction::immediates() const
    {
        return disasm::immediates(m_impl->address);
    }

    std::optional<instruction> instruction::next() const
    {
        return *this + size();
    }

    std::optional<instruction> instruction::prev() const
    {
        const auto start = m_impl->address - max_instruction_size;

        for (auto current = start; current < m_impl->address; current++)
        {
            auto instruction = at(current);

            if (!instruction)
            {
                continue;
            }

            const auto next = instruction->next();

            if (!next)
            {
                continue;
            }

            if (next->addr() != m_impl->address)
            {
                continue;
            }

            if (next->size() != size())
            {
                continue;
            }

            if (next->mnemonic() != mnemonic())
            {
                continue;
            }

            return instruction;
        }

        return std::nullopt;
    }

    std::optional<instruction> instruction::next(std::size_t mnemonic) const
    {
        auto current = next();

        while (current && current->mnemonic() != mnemonic)
        {
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            auto next = current->next();

            if (!next)
            {
                return std::nullopt;
            }

            current.emplace(std::move(next.value()));
        }

        return current;
    }

    std::optional<instruction> instruction::follow() const
    {
        auto new_address = disasm::follow(m_impl->address);

        if (!new_address)
        {
            return std::nullopt;
        }

        return at(new_address.value());
    }

    std::optional<std::uintptr_t> instruction::absolute() const
    {
        return disasm::follow(m_impl->address);
    }

    std::optional<std::uintptr_t> instruction::absolute(std::uintptr_t rip) const
    {
        return disasm::follow(m_impl->address, rip);
    }

    std::optional<instruction> instruction::operator-(std::size_t amount) const
    {
        return at(m_impl->address - amount);
    }

    std::optional<instruction> instruction::operator+(std::size_t amount) const
    {
        return at(m_impl->address + amount);
    }

    std::strong_ordering instruction::operator<=>(const instruction &other) const
    {
        const auto address = other.addr();

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

    instruction instruction::unsafe(std::uintptr_t address)
    {
        instruction rtn;

        rtn.m_impl->address = address;
        rtn.m_impl->page    = std::make_shared<lime::page>(page::unsafe(address));

        return rtn;
    }

    std::optional<instruction> instruction::at(std::uintptr_t address)
    {
        const auto page = page::at(address);

        if (!page || !(page->prot() & protection::read))
        {
            return std::nullopt;
        }

        if (!disasm::valid(address))
        {
            return std::nullopt;
        }

        return unsafe(address);
    }
} // namespace lime
