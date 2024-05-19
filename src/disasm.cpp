#include "disasm.hpp"

#include "constants.hpp"
#include "instruction.hpp"

#include <cstring>
#include <cassert>

#include <Zydis/Zydis.h>

namespace lime
{
    auto get_decoder = []()
    {
        ZydisDecoder decoder;

        if constexpr (arch == architecture::x64)
        {
            ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
        }
        else
        {
            ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);
        }

        return decoder;
    };

    bool disasm::valid(std::uintptr_t address)
    {
        const auto decoder = get_decoder();
        const auto *buffer = reinterpret_cast<void *>(address);

        ZydisDecodedInstruction inst;
        return ZYAN_SUCCESS(ZydisDecoderDecodeInstruction(&decoder, nullptr, buffer, max_instruction_size, &inst));
    }

    bool disasm::relative(std::uintptr_t address)
    {
        const auto decoder = get_decoder();
        const auto *buffer = reinterpret_cast<void *>(address);

        ZydisDecodedInstruction inst;

        if (not ZYAN_SUCCESS(ZydisDecoderDecodeInstruction(&decoder, nullptr, buffer, max_instruction_size, &inst)))
        {
            assert(false && "Failed to decode instruction");
        }

        return inst.attributes & ZYDIS_ATTRIB_IS_RELATIVE;
    }

    bool disasm::branching(std::uintptr_t address)
    {
        const auto decoder = get_decoder();
        const auto *buffer = reinterpret_cast<void *>(address);

        ZydisDecodedInstruction inst;

        if (not ZYAN_SUCCESS(ZydisDecoderDecodeInstruction(&decoder, nullptr, buffer, max_instruction_size, &inst)))
        {
            assert(false && "Failed to decode instruction");
        }

        return inst.meta.branch_type != ZYDIS_BRANCH_TYPE_NONE;
    }

    std::size_t disasm::size(std::uintptr_t address)
    {
        const auto decoder = get_decoder();
        const auto *buffer = reinterpret_cast<void *>(address);

        ZydisDecodedInstruction inst;

        if (not ZYAN_SUCCESS(ZydisDecoderDecodeInstruction(&decoder, nullptr, buffer, max_instruction_size, &inst)))
        {
            assert(false && "Failed to decode instruction");
        }

        return inst.length;
    }

    std::size_t disasm::mnemonic(std::uintptr_t address)
    {
        const auto decoder = get_decoder();
        const auto *buffer = reinterpret_cast<void *>(address);

        ZydisDecodedInstruction inst;

        if (not ZYAN_SUCCESS(ZydisDecoderDecodeInstruction(&decoder, nullptr, buffer, max_instruction_size, &inst)))
        {
            assert(false && "Failed to decode instruction");
        }

        return inst.mnemonic;
    }

    disp disasm::displacement(std::uintptr_t address)
    {
        const auto decoder = get_decoder();
        const auto *buffer = reinterpret_cast<void *>(address);

        ZydisDecodedInstruction inst;

        if (not ZYAN_SUCCESS(ZydisDecoderDecodeInstruction(&decoder, nullptr, buffer, max_instruction_size, &inst)))
        {
            assert("Failed to decode instruction" && false);
        }

        const auto &raw_disp = inst.raw.disp;
        return {.amount = raw_disp.value, .size = raw_disp.size, .offset = raw_disp.offset};
    }

    std::vector<imm> disasm::immediates(std::uintptr_t address)
    {
        const auto decoder = get_decoder();
        const auto *buffer = reinterpret_cast<void *>(address);

        ZydisDecodedInstruction inst;

        if (not ZYAN_SUCCESS(ZydisDecoderDecodeInstruction(&decoder, nullptr, buffer, max_instruction_size, &inst)))
        {
            assert(false && "Failed to decode instruction");
        }

        static constexpr auto size = sizeof(inst.raw.imm) / sizeof(inst.raw.imm[0]);

        std::vector<lime::imm> rtn;
        rtn.reserve(size);

        for (const auto &imm : inst.raw.imm)
        {
            lime::imm item{};

            item.size     = imm.size;
            item.offset   = imm.offset;
            item.relative = imm.is_relative;

            if (imm.is_signed)
            {
                item.amount = imm.value.s;
            }
            else
            {
                item.amount = imm.value.u;
            }

            rtn.emplace_back(item);
        }

        return rtn;
    }

    std::optional<std::uintptr_t> disasm::follow(std::uintptr_t address, std::optional<std::uintptr_t> rip)
    {
        const auto decoder = get_decoder();
        const auto *buffer = reinterpret_cast<void *>(address);

        ZydisDecodedInstruction inst;
        ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];

        if (not ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, buffer, max_instruction_size, &inst, operands)))
        {
            assert(false && "Failed to decode instruction");
        }

        ZyanU64 result = 0;
        ZydisCalcAbsoluteAddress(&inst, operands, rip.value_or(address), &result);

        return result;
    }
} // namespace lime
