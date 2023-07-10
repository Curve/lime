#include "disasm.hpp"

#include "constants.hpp"
#include "instruction.hpp"

#include <cstring>
#include <Zydis/Zydis.h>

namespace lime
{
    //? Max Instruction size on x86_64
    constexpr inline auto instruction_size = 0x15;

    auto get_decoder = []() {
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
        auto decoder = get_decoder();
        auto *buffer = reinterpret_cast<void *>(address);

        ZydisDecodedInstruction instruction;
        return ZYAN_SUCCESS(ZydisDecoderDecodeInstruction(&decoder, nullptr, buffer, instruction_size, &instruction));
    }

    bool disasm::relative(std::uintptr_t address)
    {
        auto decoder = get_decoder();
        auto *buffer = reinterpret_cast<void *>(address);

        ZydisDecodedInstruction instruction;

        if (not ZYAN_SUCCESS(ZydisDecoderDecodeInstruction(&decoder, nullptr, buffer, instruction_size, &instruction)))
        {
            assert(((void)"Failed to decode instruction", false));
        }

        return instruction.attributes & ZYDIS_ATTRIB_IS_RELATIVE;
    }

    bool disasm::branching(std::uintptr_t address)
    {
        auto decoder = get_decoder();
        auto *buffer = reinterpret_cast<void *>(address);

        ZydisDecodedInstruction instruction;

        if (not ZYAN_SUCCESS(ZydisDecoderDecodeInstruction(&decoder, nullptr, buffer, instruction_size, &instruction)))
        {
            assert(((void)"Failed to decode instruction", false));
        }

        return instruction.meta.branch_type != ZYDIS_BRANCH_TYPE_NONE;
    }

    std::size_t disasm::size(std::uintptr_t address)
    {
        auto decoder = get_decoder();
        auto *buffer = reinterpret_cast<void *>(address);

        ZydisDecodedInstruction instruction;

        if (not ZYAN_SUCCESS(ZydisDecoderDecodeInstruction(&decoder, nullptr, buffer, instruction_size, &instruction)))
        {
            assert(((void)"Failed to decode instruction", false));
        }

        return instruction.length;
    }

    std::size_t disasm::mnemonic(std::uintptr_t address)
    {
        auto decoder = get_decoder();
        auto *buffer = reinterpret_cast<void *>(address);

        ZydisDecodedInstruction instruction;

        if (not ZYAN_SUCCESS(ZydisDecoderDecodeInstruction(&decoder, nullptr, buffer, instruction_size, &instruction)))
        {
            assert(((void)"Failed to decode instruction", false));
        }

        return instruction.mnemonic;
    }

    disp disasm::displacement(std::uintptr_t address)
    {
        auto decoder = get_decoder();
        auto *buffer = reinterpret_cast<void *>(address);

        ZydisDecodedInstruction instruction;

        if (not ZYAN_SUCCESS(ZydisDecoderDecodeInstruction(&decoder, nullptr, buffer, instruction_size, &instruction)))
        {
            assert(((void)"Failed to decode instruction", false));
        }

        auto &raw_disp = instruction.raw.disp;
        return {.amount = raw_disp.value, .size = raw_disp.size, .offset = raw_disp.offset};
    }

    std::vector<imm> disasm::immediates(std::uintptr_t address)
    {
        auto decoder = get_decoder();
        auto *buffer = reinterpret_cast<void *>(address);

        ZydisDecodedInstruction instruction;

        if (not ZYAN_SUCCESS(ZydisDecoderDecodeInstruction(&decoder, nullptr, buffer, instruction_size, &instruction)))
        {
            assert(((void)"Failed to decode instruction", false));
        }

        constexpr auto size = sizeof(instruction.raw.imm) / sizeof(instruction.raw.imm[0]);

        std::vector<lime::imm> rtn;
        rtn.reserve(size);

        for (const auto &imm : instruction.raw.imm)
        {
            auto item = lime::imm{};

            item.size = imm.size;
            item.offset = imm.offset;
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

    std::optional<std::uintptr_t> disasm::follow(std::uintptr_t address)
    {
        auto decoder = get_decoder();
        auto *buffer = reinterpret_cast<void *>(address);

        ZydisDecodedInstruction instruction;
        ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];

        if (not ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, buffer, instruction_size, &instruction, operands)))
        {
            assert(((void)"Failed to decode instruction", false));
        }

        ZyanU64 result = 0;
        ZydisCalcAbsoluteAddress(&instruction, operands, address, &result);

        return result;
    }
} // namespace lime