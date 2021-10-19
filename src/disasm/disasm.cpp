#include "disasm.hpp"
#include <Zydis/Zydis.h>
#include <constants/architecture.hpp>
#include <cstring>
#include <utility/memory.hpp>

namespace lime
{
    std::optional<std::uintptr_t> disasm::follow(const std::uintptr_t &address)
    {
        ZydisDecoder decoder;

        if constexpr (arch == architecture::x64)
            ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
        else
            ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);

        ZydisDecodedInstruction instruction;
        if (ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(&decoder, reinterpret_cast<void *>(address), 16, &instruction)))
        {
            ZyanU64 result = 0;
            ZydisCalcAbsoluteAddress(&instruction, instruction.operands, address, &result);

            return result;
        }

        return std::nullopt;
    }

    std::optional<std::uint32_t> disasm::get_mnemonic(const std::uintptr_t &address)
    {
        ZydisDecoder decoder;

        if constexpr (arch == architecture::x64)
            ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
        else
            ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);

        ZydisDecodedInstruction instruction;
        if (ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(&decoder, reinterpret_cast<void *>(address), 16, &instruction)))
        {
            return instruction.mnemonic;
        }

        return std::nullopt;
    }

    std::optional<std::uintptr_t> disasm::read_until(const std::uint32_t &mnemonic, const std::uintptr_t &address, const std::size_t &size)
    {
        ZydisDecoder decoder;

        if constexpr (arch == architecture::x64)
            ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
        else
            ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);

        std::size_t offset = 0;
        ZydisDecodedInstruction instruction;

        while (ZYAN_SUCCESS(
            ZydisDecoderDecodeBuffer(&decoder, reinterpret_cast<void *>(address + offset), std::min<std::size_t>(16, size - offset), &instruction)))
        {
            if (instruction.mnemonic == mnemonic)
            {
                return address + offset;
            }

            offset += instruction.length;
        }

        return std::nullopt;
    }

    bool disasm::is_far_relocateable(const std::vector<std::uint8_t> &code)
    {
        if constexpr (arch == architecture::x86)
            return true;

        ZydisDecoder decoder;
        ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);

        std::size_t offset = 0;
        ZydisDecodedInstruction instruction;

        // clang-format off
        while (ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(&decoder, code.data() + offset, std::min<std::size_t>(code.size() - offset, 16), &instruction)))
        {
            if (instruction.attributes & ZYDIS_ATTRIB_IS_RELATIVE)
            {
                if (instruction.meta.branch_type == ZYDIS_BRANCH_TYPE_NONE)
                {
                    return false;
                }
            }

            offset += instruction.length;
        }
        // clang-format on

        return true;
    }

    std::size_t disasm::get_required_prologue_length(const std::uintptr_t &where, const std::size_t &desired_size)
    {
        ZydisDecoder decoder;

        if constexpr (arch == architecture::x64)
            ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
        else
            ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);

        std::size_t offset = 0;
        ZydisDecodedInstruction instruction;

        while (ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(&decoder, reinterpret_cast<void *>(where + offset), 16, &instruction)))
        {
            offset += instruction.length;
            if (offset >= desired_size)
                return offset;
        }

        return 0;
    }

    template <typename target_t> bool safe_add(const std::uintptr_t &address, const std::int64_t &offset)
    {
        auto &target = *reinterpret_cast<target_t *>(address);
        if ((static_cast<std::int64_t>(target) + offset) == (target + static_cast<target_t>(offset)))
        {
            target += offset;
            return true;
        }

        return false;
    }

    bool compensate(const std::uintptr_t &address, const std::size_t &size, const std::int64_t &offset)
    {
        switch (size)
        {
        case sizeof(std::int8_t) * 8:
            return safe_add<std::int8_t>(address, offset);
        case sizeof(std::int16_t) * 8:
            return safe_add<std::int16_t>(address, offset);
        case sizeof(std::int32_t) * 8:
            return safe_add<std::int32_t>(address, offset);
        case sizeof(std::int64_t) * 8:
            return safe_add<std::int64_t>(address, offset);
        default:
            return false;
        }
    }

    std::size_t disasm::get_estimated_size(const std::vector<std::uint8_t> &code, const bool &far)
    {
        ZydisDecoder decoder;

        if constexpr (arch == architecture::x64)
            ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
        else
            ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);

        std::size_t rtn = 0;
        std::size_t offset = 0;
        ZydisDecodedInstruction instruction;
        constexpr auto jump_table_entry_size = arch == architecture::x64 ? (sizeof(std::uintptr_t) + 6) : (sizeof(std::uintptr_t) + 1);

        auto raw_code = std::make_unique<char[]>(code.size());
        memcpy(raw_code.get(), code.data(), code.size());

        while (ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(&decoder, raw_code.get() + offset, std::min<std::size_t>(code.size() - offset, 16), &instruction)))
        {
            if (instruction.attributes & ZYDIS_ATTRIB_IS_RELATIVE)
            {
                for (int i = 0; 2 > i; i++)
                {
                    if (instruction.meta.branch_type != ZYDIS_BRANCH_TYPE_NONE && instruction.raw.imm[i].is_relative)
                    {
                        if (!far && (instruction.raw.imm[i].size < sizeof(std::int32_t) * 8))
                        {
                            rtn += jump_table_entry_size;
                        }

                        if (far && (instruction.raw.imm[i].size < sizeof(std::int64_t) * 8))
                        {
                            rtn += jump_table_entry_size;
                        }
                    }
                }
            }

            offset += instruction.length;
        }

        return rtn + code.size() + jump_table_entry_size * 2;
    }

    std::optional<std::vector<std::uint8_t>> disasm::build_code(const std::vector<std::uint8_t> &code, const std::uintptr_t &hook,
                                                                const std::uintptr_t &old_pos, const std::uintptr_t &new_pos)
    {
        ZydisDecoder decoder;

        if constexpr (arch == architecture::x64)
            ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
        else
            ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);

        std::size_t offset = 0;
        ZydisDecodedInstruction instruction;

        auto raw_code = std::make_unique<char[]>(code.size());
        memcpy(raw_code.get(), code.data(), code.size());

        std::vector<std::uint8_t> jump_table;
        constexpr auto jmp_size = arch == architecture::x64 ? 6 + sizeof(std::uintptr_t) : 1 + sizeof(std::uintptr_t);
        const auto diff = static_cast<std::int64_t>(old_pos) - static_cast<std::int64_t>(new_pos) - static_cast<std::int64_t>(jmp_size);

        while (ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(&decoder, raw_code.get() + offset, std::min<std::size_t>(code.size() - offset, 16), &instruction)))
        {
            auto pos = reinterpret_cast<std::uintptr_t>(raw_code.get() + offset);

            if (instruction.attributes & ZYDIS_ATTRIB_IS_RELATIVE)
            {
                if (instruction.raw.disp.offset)
                {
                    if (!compensate(pos + instruction.raw.disp.offset, instruction.raw.disp.size, diff))
                    {
                        return std::nullopt;
                    }
                }

                for (int i = 0; 2 > i; i++)
                {
                    if (instruction.raw.imm[i].offset && instruction.raw.imm[i].is_relative)
                    {
                        if (!compensate(pos + instruction.raw.imm[i].offset, instruction.raw.imm[i].size, diff))
                        {
                            if (instruction.meta.branch_type != ZYDIS_BRANCH_TYPE_NONE && instruction.raw.imm[i].is_relative)
                            {
                                auto original_destination = old_pos + offset + instruction.length;
                                const auto jump_table_offset = (jump_table.size() / jmp_size) * jmp_size - jmp_size;

                                if (instruction.raw.imm[i].is_signed)
                                    original_destination += instruction.raw.imm[i].value.s;
                                else
                                    original_destination += instruction.raw.imm[i].value.u;

                                if constexpr (arch == architecture::x64)
                                {
                                    jump_table.insert(jump_table.end(), {0xFF, 0x25, 0x00, 0x00, 0x00, 0x00});
                                    jump_table.resize(jump_table.size() + sizeof(std::uintptr_t));

                                    *reinterpret_cast<std::uintptr_t *>(jump_table.data() + jump_table.size() - sizeof(std::uintptr_t)) =
                                        original_destination;
                                }
                                else
                                {
                                    jump_table.insert(jump_table.end(), 0xE9);
                                    jump_table.resize(jump_table.size() + sizeof(std::intptr_t));
                                    *reinterpret_cast<std::intptr_t *>(jump_table.data() + jump_table.size() - sizeof(std::intptr_t)) =
                                        static_cast<std::intptr_t>(original_destination) -
                                        static_cast<std::intptr_t>(new_pos + code.size() + 1 + sizeof(std::intptr_t) + jump_table.size()) - 5;
                                }

                                const auto disp = static_cast<std::int64_t>(
                                    -instruction.raw.imm[i].value.s + (code.size() - offset - instruction.length + jmp_size + jump_table_offset + 5));

                                if (!compensate(pos + instruction.raw.imm[i].offset, instruction.raw.imm[i].size, disp))
                                {
                                    return std::nullopt;
                                }
                            }
                            else
                            {
                                return std::nullopt;
                            }
                        }
                    }
                }
            }

            offset += instruction.length;
        }

        std::vector<std::uint8_t> rtn(raw_code.get(), raw_code.get() + code.size());

        if constexpr (arch == architecture::x64)
        {
            rtn.insert(rtn.begin(), {0xFF, 0x25, 0x00, 0x00, 0x00, 0x00});
            rtn.insert(rtn.begin() + 6, sizeof(std::uintptr_t), 0x00);
            *reinterpret_cast<std::uintptr_t *>(rtn.data() + 6) = hook;

            rtn.insert(rtn.end(), {0xFF, 0x25, 0x00, 0x00, 0x00, 0x00});
            rtn.resize(rtn.size() + sizeof(std::uintptr_t));
            *reinterpret_cast<std::uintptr_t *>(rtn.data() + rtn.size() - sizeof(std::uintptr_t)) = old_pos + code.size();
        }
        else
        {
            rtn.insert(rtn.begin(), 0xE9);
            rtn.insert(rtn.begin() + 1, sizeof(std::uintptr_t), 0x00);
            *reinterpret_cast<std::uintptr_t *>(rtn.data() + 1) = static_cast<std::intptr_t>(hook) - static_cast<std::intptr_t>(new_pos) - 5;

            rtn.insert(rtn.end(), 0xE9);
            rtn.insert(rtn.end(), sizeof(std::intptr_t), 0x90);
            *reinterpret_cast<std::intptr_t *>(rtn.data() + rtn.size() - sizeof(std::intptr_t)) =
                static_cast<std::intptr_t>(old_pos) - static_cast<std::intptr_t>(new_pos + rtn.size()) + static_cast<std::intptr_t>(code.size());
        }

        rtn.insert(rtn.end(), jump_table.begin(), jump_table.end());
        return rtn;
    }
} // namespace lime