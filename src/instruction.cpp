#include "instruction.hpp"

#include "address.hpp"
#include "constants.hpp"

#include <ranges>
#include <cstring>

#include <Zydis/Zydis.h>

namespace lime
{
    struct instruction::impl
    {
        std::uintptr_t address;

      public:
        ZydisDecodedInstruction instruction;
        std::array<ZydisDecodedOperand, ZYDIS_MAX_OPERAND_COUNT> operands;
    };

    instruction::instruction(impl data) : m_impl(std::make_unique<impl>(data)) {}

    instruction::instruction(const instruction &other) : instruction(*other.m_impl) {}

    instruction::instruction(instruction &&other) noexcept : m_impl(std::move(other.m_impl)) {}

    instruction::~instruction() = default;

    instruction &instruction::operator=(instruction other) noexcept
    {
        swap(*this, other);
        return *this;
    }

    void swap(instruction &first, instruction &second) noexcept
    {
        using std::swap;
        swap(first.m_impl, second.m_impl);
    }

    std::uintptr_t instruction::address() const
    {
        return m_impl->address;
    }

    std::size_t instruction::size() const
    {
        return m_impl->instruction.length;
    }

    lime::mnemonic instruction::mnemonic() const
    {
        return ZydisMnemonicGetString(m_impl->instruction.mnemonic);
    }

    bool instruction::relative() const
    {
        return m_impl->instruction.attributes & ZYDIS_ATTRIB_IS_RELATIVE;
    }

    bool instruction::branching() const
    {
        return m_impl->instruction.meta.branch_type != ZYDIS_BRANCH_TYPE_NONE;
    }

    disp instruction::displacement() const
    {
        const auto &displacement = m_impl->instruction.raw.disp;

        return {
            .amount = displacement.value,
            .size   = displacement.size,
            .offset = displacement.offset,
        };
    }

    std::array<imm, 2> instruction::immediates() const
    {
        static constexpr auto transform = [](auto value) -> imm
        {
            return {
                .relative = static_cast<bool>(value.is_relative),
                .amount   = value.is_signed ? value.value.s : value.value.u, // NOLINT(*-union-access)
                .size     = value.size,
                .offset   = value.offset,
            };
        };

        return {transform(m_impl->instruction.raw.imm[0]), transform(m_impl->instruction.raw.imm[1])};
    }

    std::optional<instruction> instruction::next() const
    {
        return operator+(size());
    }

    std::optional<instruction> instruction::next(lime::mnemonic mnemonic) const
    {
        for (auto it = next(); it; it = it->next())
        {
            if (it->mnemonic() != mnemonic)
            {
                continue;
            }

            return it;
        }

        return std::nullopt;
    }

    std::vector<instruction> instruction::prev() const
    {
        const auto address = m_impl->address;
        const auto start   = address - max_instruction_size;

        auto rtn = std::vector<instruction>{};

        for (auto current = start; current < address; ++current)
        {
            auto instruction = at(current);

            if (!instruction.has_value())
            {
                continue;
            }

            const auto next = instruction->next();

            if (!next.has_value())
            {
                continue;
            }

            if (next->address() != address)
            {
                continue;
            }

            auto *self  = reinterpret_cast<void *>(address);
            auto *other = reinterpret_cast<void *>(next->address());

            if (std::memcmp(self, other, size()) != 0)
            {
                continue;
            }

            rtn.emplace_back(std::move(*instruction));
        }

        return rtn;
    }

    std::vector<instruction> instruction::prev(lime::mnemonic mnemonic) const
    {
        return prev()                                                                              //
               | std::views::filter([&](const auto &item) { return item.mnemonic() == mnemonic; }) //
               | std::ranges::to<std::vector>();
    }

    std::optional<std::uintptr_t> instruction::follow() const
    {
        return follow(m_impl->address);
    }

    std::optional<std::uintptr_t> instruction::follow(std::uintptr_t rip) const
    {
        for (auto i = 0uz; m_impl->operands.size() > i; ++i)
        {
            auto rtn = follow(rip, i);

            if (!rtn.has_value())
            {
                continue;
            }

            return rtn;
        }

        return std::nullopt;
    }

    std::optional<std::uintptr_t> instruction::follow(std::uintptr_t rip, std::size_t operand) const
    {
        if (operand >= m_impl->operands.size())
        {
            return std::nullopt;
        }

        auto result = ZyanU64{};

        if (!ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(&m_impl->instruction, &m_impl->operands.at(operand), rip, &result)))
        {
            return std::nullopt;
        }

        return result;
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
        return address() <=> other.address();
    }

    std::optional<instruction> instruction::at(std::uintptr_t address)
    {
        const auto addr = lime::address::at(address);

        if (!addr)
        {
            return std::nullopt;
        }

        return at(*addr);
    }

    std::optional<instruction> instruction::at(const lime::address &addr)
    {
        auto decoder = ZydisDecoder{};

        if constexpr (arch == architecture::x64)
        {
            ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
        }
        else
        {
            ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);
        }

        auto inst = ZydisDecodedInstruction{};
        auto ops  = std::array<ZydisDecodedOperand, ZYDIS_MAX_OPERAND_COUNT>{};

        if (!ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, addr.ptr(), max_instruction_size, &inst, ops.data())))
        {
            return std::nullopt;
        }

        return instruction{{.address = addr.value(), .instruction = inst, .operands = ops}};
    }
} // namespace lime
