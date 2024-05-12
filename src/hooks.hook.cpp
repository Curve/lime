#include "hooks/hook.hpp"

#include "page.hpp"
#include "address.hpp"
#include "constants.hpp"
#include "instruction.hpp"

#include <cassert>

namespace lime
{
    struct hook_base::impl
    {
        std::uintptr_t target;

      public:
        std::unique_ptr<address> source;
        std::unique_ptr<page> source_page;

      public:
        std::vector<std::uint8_t> prologue;

      public:
        std::shared_ptr<page> spring_board;
        [[nodiscard]] std::size_t create_springboard();

      public:
        std::shared_ptr<page> trampoline;
        [[nodiscard]] bool build_trampoline();

      public:
        template <typename T>
        [[nodiscard]] bool try_offset(std::uintptr_t, std::int64_t);
        [[nodiscard]] bool try_offset(imm, std::uintptr_t, std::int64_t);
        [[nodiscard]] bool try_offset(disp, std::uintptr_t, std::int64_t);

      public:
        [[nodiscard]] bool can_relocate_far();
        [[nodiscard]] std::size_t estimate_size(bool);

      public:
        static std::vector<std::uint8_t> make_jmp(std::uintptr_t, std::uintptr_t, bool = false);
    };

    hook_base::hook_base() : m_impl(std::make_unique<impl>()) {}

    hook_base::~hook_base()
    {
        if (!m_impl)
        {
            return;
        }

        if (!m_impl->source_page->protect(protection::read | protection::write | protection::execute))
        {
            assert(false && "Failed to protect original function for restoration");
            return;
        }

        m_impl->source->write(m_impl->prologue.data(), m_impl->prologue.size());
        m_impl->source_page->restore();
    }

    std::uintptr_t hook_base::original() const
    {
        return m_impl->trampoline->start();
    }

    hook_base::rtn_t hook_base::create(std::uintptr_t source, std::uintptr_t target)
    {
        auto rtn  = std::unique_ptr<hook_base>(new hook_base);
        auto page = page::at(source);

        if (!page)
        {
            return tl::make_unexpected(hook_error::bad_page);
        }

        auto start = instruction::unsafe(source);

        if (auto follow = start.follow(); follow)
        {
            start = std::move(follow.value());
            page.emplace(page::unsafe(start));
        }

        rtn->m_impl->source      = std::make_unique<address>(address::unsafe(std::move(start)));
        rtn->m_impl->source_page = std::make_unique<lime::page>(std::move(page.value()));
        rtn->m_impl->target      = target;

        /*
        # We will now try to create a springboard and check how many bytes of the prologue we'll have to overwrite
        # ├ Worst-Case
        # │  └ We can't allocate a spring-board, size will be size::jmp_far
        # │
        # └ Best-Case
        #    └ We can allocate a spring-board, size will be size::jmp_near

        # The springboard is used to jump to the target function.
        # The idea here is just that our module is probably too far away from our target, so we just use an absolute jmp
        # for simplicity sake. As we wan't to override as little instructions as possible in the source function we use
        # the springboard.
        */

        const auto prologue_size = rtn->m_impl->create_springboard();
        rtn->m_impl->prologue    = rtn->m_impl->source->copy(prologue_size);

        /*
        # We will now try to relocate the function prologue to our trampoline
        */

        if (!rtn->m_impl->build_trampoline())
        {
            return tl::make_unexpected(hook_error::relocate);
        }

        if (rtn->m_impl->spring_board)
        {
            auto jmp = impl::make_jmp(rtn->m_impl->spring_board->start(), target);
            address::unsafe(rtn->m_impl->spring_board->start()).write(jmp.data(), jmp.size());
        }

        /*
        # Now we'll jump to the springboard/target
        */

        const auto destination = rtn->m_impl->spring_board ? rtn->m_impl->spring_board->start() : target;
        const auto near        = static_cast<bool>(rtn->m_impl->spring_board);

        auto jmp = impl::make_jmp(*rtn->m_impl->source, destination, near);

        if (auto remaining = static_cast<int>(prologue_size - jmp.size()); remaining > 0)
        {
            std::vector<std::uint8_t> nop(remaining, 0x90);
            jmp.insert(jmp.end(), nop.begin(), nop.end());
        }

        static constexpr auto prot = protection::read | protection::write | protection::execute;

        if (!rtn->m_impl->source_page->protect(prot))
        {
            return tl::make_unexpected(hook_error::protect);
        }

        rtn->m_impl->source->write(jmp.data(), jmp.size());
        rtn->m_impl->source_page->restore();

        return rtn;
    }

    std::size_t hook_base::impl::create_springboard()
    {
        static constexpr auto prot = protection::read | protection::write | protection::execute;
        spring_board               = page::allocate<alloc_policy::nearby>(*source, size::jmp_far, prot);

        const auto required_size = spring_board ? size::jmp_near : size::jmp_far;
        auto rtn                 = 0u;

        auto current = instruction::unsafe(*source);

        while (required_size > rtn)
        {
            rtn += current.size();

            auto next = current.next();
            assert(next && "Failed to decode prologue instruction");

            current = std::move(next.value());
        }

        return rtn;
    }

    bool hook_base::impl::build_trampoline()
    {
        static constexpr auto prot = protection::read | protection::write | protection::execute;

        const auto near = !can_relocate_far();
        const auto size = estimate_size(near);

        if (near)
        {
            trampoline = page::allocate<alloc_policy::nearby>(*source, size, prot);
        }
        else
        {
            trampoline = page::allocate(size, prot);
        }

        if (!trampoline)
        {
            return false;
        }

        /*
        # First, we write the prologue to our trampoline
        */

        const auto skip      = spring_board ? size::jmp_near : size::jmp_far;
        const auto jump_back = impl::make_jmp(trampoline->start() + prologue.size(), source->addr() + skip, near);

        auto body = prologue;
        body.insert(body.end(), jump_back.begin(), jump_back.end());

        address::unsafe(trampoline->start()).write(body.data(), body.size());

        /*
        # Next, we fix-up the relocated instructions
        */

        const auto prologue_end = address::unsafe(trampoline->start() + prologue.size() + size::jmp_far);
        std::vector<std::uint8_t> jump_table;

        auto i = 0u;

        while (prologue.size() > i)
        {
            auto current = instruction::unsafe(trampoline->start() + i);
            i += current.size();

            if (!current.relative())
            {
                continue;
            }

            /*
            # We now calculate the difference between the old instructions location and its new location
            # Using this, we can try to correct relative instructions, by subtracting the difference
            # from their relative-amount.
            */

            const auto original_rip = static_cast<std::int64_t>(*source) + i;
            const auto difference   = static_cast<std::int64_t>(current.addr() + current.size()) - original_rip;

            const auto disp = current.displacement();

            if (disp.size > 0 && !try_offset(disp, current, difference))
            {
                return false;
            }

            const auto immediates = current.immediates();

            for (const auto &imm : immediates)
            {
                if (imm.size <= 0 || !imm.relative)
                {
                    continue;
                }

                if (try_offset(imm, current, difference))
                {
                    continue;
                }

                if (!current.branching())
                {
                    return false;
                }

                /*
                # We failed to offset, thus we have to fall back to a jump-table at the end of the trampoline
                */

                const auto imm_value =
                    std::visit([](auto amount) { return static_cast<std::int64_t>(amount); }, imm.amount);

                const auto rel_jump = (prologue_end.addr() - current.addr() - current.size()) + jump_table.size();
                const auto offset   = -imm_value + static_cast<std::int64_t>(rel_jump);

                const auto destination = original_rip + imm_value;
                const auto table_entry = impl::make_jmp(current, destination, near);

                jump_table.insert(jump_table.end(), table_entry.begin(), table_entry.end());

                if (!try_offset(imm, current, -offset))
                {
                    return false;
                }
            }
        }

        address::unsafe(prologue_end).write(jump_table.data(), jump_table.size());

        return true;
    }

    template <typename T>
    bool hook_base::impl::try_offset(std::uintptr_t address, std::int64_t amount)
    {
        auto &original = *reinterpret_cast<T *>(address);

        const auto first  = original - static_cast<T>(amount);
        const auto second = static_cast<std::int64_t>(original) - amount;

        if (first != second)
        {
            return false;
        }

        original -= amount;
        return true;
    }

    bool hook_base::impl::try_offset(imm value, std::uintptr_t address, std::int64_t amount)
    {
        const auto is_signed = std::holds_alternative<std::int64_t>(value.amount);
        const auto target    = address + value.offset;

        switch (value.size)
        {
        case 8:
            return is_signed ? try_offset<std::int8_t>(target, amount) : try_offset<std::uint8_t>(target, amount);
        case 16:
            return is_signed ? try_offset<std::int16_t>(target, amount) : try_offset<std::uint16_t>(target, amount);
        case 32:
            return is_signed ? try_offset<std::int32_t>(target, amount) : try_offset<std::uint32_t>(target, amount);
        }

        return false;
    }

    bool hook_base::impl::try_offset(disp value, std::uintptr_t address, std::int64_t amount)
    {
        const auto target = address + value.offset;

        switch (value.size)
        {
        case 8:
            return try_offset<std::int8_t>(target, amount);
        case 16:
            return try_offset<std::int16_t>(target, amount);
        case 32:
            return try_offset<std::int32_t>(target, amount);
        }

        return false;
    }

    bool hook_base::impl::can_relocate_far()
    {
        auto i = 0u;

        while (prologue.size() > i)
        {
            const auto current = instruction::unsafe(reinterpret_cast<std::uintptr_t>(prologue.data() + i));
            i += current.size();

            if (!current.relative())
            {
                continue;
            }

            if (current.branching())
            {
                continue;
            }

            return false;
        }

        return true;
    }

    std::size_t hook_base::impl::estimate_size(bool near)
    {
        const auto size = size::jmp_far + prologue.size();

        auto i = 0u;

        while (prologue.size() > i)
        {
            const auto current = instruction::unsafe(reinterpret_cast<std::uintptr_t>(prologue.data() + i));
            i += current.size();

            if (!current.relative())
            {
                continue;
            }

            i += near ? size::jmp_near : size::jmp_far;
        }

        return size;
    }

    std::vector<std::uint8_t> hook_base::impl::make_jmp(std::uintptr_t from, std::uintptr_t to, bool near)
    {
        std::vector<std::uint8_t> rtn;

        if (near || arch == architecture::x86)
        {
            rtn.insert(rtn.end(), {0xE9});

            rtn.resize(rtn.size() + sizeof(std::int32_t));
            *reinterpret_cast<std::int32_t *>(rtn.data() + 1) = static_cast<std::int32_t>(to - from - size::jmp_near);
        }
        else
        {
            rtn.insert(rtn.end(), {0xFF, 0x25, 0x00, 0x00, 0x00, 0x00});

            rtn.resize(rtn.size() + sizeof(std::uintptr_t));
            *reinterpret_cast<std::uintptr_t *>(rtn.data() + 6) = to;
        }

        return rtn;
    }
} // namespace lime
