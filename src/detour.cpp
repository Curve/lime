#include "hooks/detour.hpp"
#include "disasm/disasm.hpp"
#include "utility/memory.hpp"
#include "constants/mnemonics.hpp"
#include "constants/protection.hpp"
#include "constants/architecture.hpp"

#include <cstring>

namespace lime
{
    detour::detour() = default;

    detour::~detour()
    {
        if (protect(m_original_page->get_start(), m_original_page->get_end() - m_original_page->get_start(), prot::read_write_execute))
        {
            write(m_target, m_original_code.data(), m_original_code.size());
            protect(m_original_page->get_start(), m_original_page->get_end() - m_original_page->get_start(), m_original_page->get_protection());
        }
    }

    std::uintptr_t detour::get_original() const
    {
        return m_original_func;
    }

    std::unique_ptr<detour> detour::create(std::uintptr_t target, const std::uintptr_t &replacement)
    {
        [[maybe_unused]] detour_status status{};
        return create(target, replacement, status);
    }

    std::unique_ptr<detour> detour::create(std::uintptr_t target, const std::uintptr_t &replacement, detour_status &status)
    {
        auto original_page = page::get_page_at(target);

        if (!original_page)
        {
            status = detour_status::invalid_page;
            return nullptr;
        }

        if (original_page->get_protection() != prot::none && original_page->get_protection() != prot::execute)
        {
            if (disasm::get_mnemonic(target) == mnemonic::JMP)
            {
                auto follow = disasm::follow(target);

                if (follow)
                {
                    target = *follow;

                    auto new_page = page::get_page_at(target);

                    if (!new_page)
                    {
                        status = detour_status::invalid_page;
                        return nullptr;
                    }

                    original_page.emplace(*new_page);
                }
            }
        }

        if (!protect(original_page->get_start(), original_page->get_end() - original_page->get_start(), prot::read_write_execute))
        {
            status = detour_status::could_not_protect;
            return nullptr;
        }

        auto required_size = disasm::get_required_prologue_length(target, 6);

        auto raw_original = read(target, required_size);
        std::vector<std::uint8_t> original_code(raw_original.get(), raw_original.get() + required_size);

        const auto is_relocateable = disasm::is_far_relocateable(original_code);
        auto estimated_size = disasm::get_estimated_size(original_code, false);
        auto trampoline_page = allocate_near(target, estimated_size, prot::read_write_execute);

        std::vector<std::uint8_t> fixed_code;
        bool is_near = true;

        if (!trampoline_page)
        {
            if (is_relocateable)
            {
                required_size = disasm::get_required_prologue_length(target, arch == architecture::x64 ? (6 + sizeof(std::uintptr_t)) : (1 + sizeof(std::uintptr_t)));

                raw_original = read(target, required_size);
                original_code = {raw_original.get(), raw_original.get() + required_size};

                estimated_size = disasm::get_estimated_size(original_code, true);
                trampoline_page = allocate(estimated_size, prot::read_write_execute);

                if (trampoline_page)
                {
                    const auto fixed = disasm::build_code(original_code, replacement, target, *trampoline_page);
                    if (fixed)
                    {
                        fixed_code = *fixed;
                        is_near = false;
                    }
                }
            }
        }
        else
        {
            const auto fixed = disasm::build_code(original_code, replacement, target, *trampoline_page);
            if (fixed)
            {
                fixed_code = *fixed;
            }
        }

        std::unique_ptr<detour> rtn;
        if (!fixed_code.empty() && fixed_code.size() <= estimated_size)
        {
            write(*trampoline_page, fixed_code.data(), fixed_code.size());

            if (is_near || arch == architecture::x86)
            {
                std::vector<std::uint8_t> hook_code(original_code.size(), 0x90);

                hook_code.front() = 0xE9;
                *reinterpret_cast<std::int32_t *>(hook_code.data() + 1) = static_cast<std::int32_t>(*trampoline_page - target) - 5;

                write(target, hook_code.data(), hook_code.size());
            }
            else if (!is_near)
            {
                std::vector<std::uint8_t> hook_code(original_code.size(), 0x90);

                hook_code[0] = 0xFF;
                hook_code[1] = 0x25;
                hook_code[2] = 0x00;
                hook_code[3] = 0x00;
                hook_code[4] = 0x00;
                hook_code[5] = 0x00;
                *reinterpret_cast<std::uintptr_t *>(hook_code.data() + 6) = *trampoline_page;

                write(target, hook_code.data(), hook_code.size());
            }

            status = detour_status::success;
            rtn = std::unique_ptr<detour>(new detour);

            rtn->m_target = target;
            rtn->m_replacement = replacement;
            rtn->m_trampoline = trampoline_page;
            rtn->m_original_code = original_code;
            rtn->m_original_page = std::make_unique<lime::page>(*original_page);
            rtn->m_original_func = *trampoline_page + (arch == architecture::x64 ? 6 + sizeof(std::uintptr_t) : 1 + sizeof(std::uintptr_t));
        }
        else
        {
            status = detour_status::could_not_relocate;
        }

        protect(original_page->get_start(), original_page->get_end() - original_page->get_start(), original_page->get_protection());
        return rtn;
    }
} // namespace lime