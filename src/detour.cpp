#include "disasm/disasm.hpp"
#include <constants/architecture.hpp>
#include <constants/protection.hpp>
#include <cstring>
#include <hooks/detour.hpp>
#include <utility/memory.hpp>

namespace lime
{
    detour::detour() = default;
    detour::~detour() = default;
    detour::detour(detour &&) noexcept = default;

    std::uintptr_t detour::get_original() const
    {
        return m_original_func;
    }

    std::optional<detour> detour::create(const std::uintptr_t &target, const std::uintptr_t &replacement)
    {
        auto original_page = page::get_page_at(target);

        if (!original_page)
            return std::nullopt;

        if (!protect(original_page->get_start(), original_page->get_end() - original_page->get_start(), prot_read | prot_write | prot_execute))
            return std::nullopt;

        auto required_size = disasm::get_required_prologue_length(target, 6);

        auto *buffer = new std::uint8_t[required_size];
        memcpy(buffer, reinterpret_cast<void *>(target), required_size);

        std::vector<std::uint8_t> original_code(buffer, buffer + required_size);
        delete[] buffer;

        auto is_relocateable = disasm::is_far_relocateable(original_code);
        auto estimated_size = disasm::get_estimated_size(original_code, false);
        auto trampoline_page = allocate_near(target, estimated_size, prot_read | prot_write | prot_execute);

        std::vector<std::uint8_t> fixed_code;
        bool is_near = true;

        if (!trampoline_page)
        {
            if (is_relocateable)
            {
                required_size = disasm::get_required_prologue_length(target, arch == architecture::x64 ? (6 + sizeof(std::uintptr_t))
                                                                                                       : (1 + sizeof(std::uintptr_t)));

                buffer = new std::uint8_t[required_size];
                memcpy(buffer, reinterpret_cast<void *>(target), required_size);

                original_code = {buffer, buffer + required_size};
                delete[] buffer;

                estimated_size = disasm::get_estimated_size(original_code, true);
                trampoline_page = allocate(estimated_size, prot_read | prot_write | prot_execute);

                if (trampoline_page)
                {
                    auto fixed = disasm::build_code(original_code, replacement, target, *trampoline_page);
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
            auto fixed = disasm::build_code(original_code, replacement, target, *trampoline_page);
            if (fixed)
            {
                fixed_code = *fixed;
            }
        }

        std::optional<detour> rtn;

        if (!fixed_code.empty() && fixed_code.size() <= estimated_size)
        {
            memcpy(reinterpret_cast<void *>(*trampoline_page), fixed_code.data(), fixed_code.size());

            if (is_near || arch == architecture::x86)
            {
                std::vector<std::uint8_t> hook_code(original_code.size(), 0x90);

                hook_code.front() = 0xE9;
                *reinterpret_cast<std::int32_t *>(hook_code.data() + 1) = static_cast<std::int32_t>(*trampoline_page - target) - 5;

                memcpy(reinterpret_cast<void *>(target), hook_code.data(), hook_code.size());
            }
            else if (!is_near)
            {
                std::vector<std::uint8_t> hook_code(original_code.size(), 0x90);

                hook_code[0] = 0xFF;
                hook_code[1] = 0x25;
                *reinterpret_cast<std::uintptr_t *>(hook_code.data() + 2) = *trampoline_page;

                memcpy(reinterpret_cast<void *>(target), hook_code.data(), hook_code.size());
            }

            rtn.emplace(detour{});
            rtn->m_target = target;
            rtn->m_replacement = replacement;
            rtn->m_trampoline = trampoline_page;
            rtn->m_original_code = original_code;
            rtn->m_original_page = std::make_unique<lime::page>(std::move(*original_page));
            rtn->m_original_func = *trampoline_page + (arch == architecture::x64 ? 6 + sizeof(std::uintptr_t) : 1 + sizeof(std::uintptr_t));

            return rtn;
        }

        protect(original_page->get_start(), original_page->get_end() - original_page->get_start(), original_page->get_protection());
        return rtn;
    }
} // namespace lime