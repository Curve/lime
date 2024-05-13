#include "hooks/hook.hpp"

#include "page.hpp"
#include "address.hpp"
#include "constants.hpp"
#include "instruction.hpp"
#include "tl/expected.hpp"

#include <limits>
#include <cassert>
#include <iterator>
#include <algorithm>

namespace lime
{
    using offset_ptr = std::variant<std::int8_t *,   //
                                    std::uint8_t *,  //
                                    std::int16_t *,  //
                                    std::uint16_t *, //
                                    std::int32_t *,  //
                                    std::uint32_t *  //
                                    >;

    static constexpr auto rwx = protection::read | protection::write | protection::execute;

    struct hook_base::impl
    {
        std::uintptr_t target;

      public:
        std::unique_ptr<address> source;
        std::shared_ptr<page> source_page;
        std::vector<std::uint8_t> prologue;

      public:
        std::shared_ptr<page> trampoline;
        std::shared_ptr<page> spring_board;

      public:
        [[nodiscard]] std::size_t required_prologue_size(bool near) const;
        [[nodiscard]] std::size_t estimate_trampoline_size(bool near) const;

      public:
        bool create_springboard();
        bool create_trampoline(bool near, bool spring_board);

      public:
        static std::optional<offset_ptr> offset_of(const instruction &source);
        static std::optional<offset_ptr> offset_of(std::uintptr_t, const disp &);
        static std::optional<offset_ptr> offset_of(std::uintptr_t, const std::vector<imm> &);

      public:
        static bool try_offset(const offset_ptr &source, std::intptr_t offset);
        static bool try_redirect(const offset_ptr &source, std::intptr_t target);

      public:
        static std::vector<std::uint8_t> make_ptr(std::uintptr_t address);
        static std::vector<std::uint8_t> make_jmp(std::uintptr_t source, std::uintptr_t target, bool near = false);
    };

    hook_base::hook_base() : m_impl(std::make_unique<impl>()) {}

    hook_base::~hook_base()
    {
        if (!m_impl || !m_impl->source_page)
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
        auto page = page::at(source);

        if (!page)
        {
            return tl::make_unexpected(hook_error::bad_page);
        }

        if (!(page->prot() & protection::read))
        {
            return tl::make_unexpected(hook_error::bad_prot);
        }

        auto start = instruction::at(source);

        if (!start)
        {
            return tl::make_unexpected(hook_error::bad_func);
        }

        if (auto follow = start->follow(); follow)
        {
            start = std::move(follow.value());
            page.emplace(page::unsafe(start->addr()));
        }

        auto rtn = std::unique_ptr<hook_base>(new hook_base);

        rtn->m_impl->target      = target;
        rtn->m_impl->source_page = std::make_unique<lime::page>(std::move(page.value()));
        rtn->m_impl->source      = std::make_unique<address>(address::unsafe(start->addr()));

        const auto spring_board = rtn->m_impl->create_springboard();
        const auto near         = rtn->m_impl->create_trampoline(true, spring_board);

        if (!near && !rtn->m_impl->create_trampoline(false, spring_board))
        {
            return tl::make_unexpected(hook_error::relocate);
        }

        const auto destination = spring_board ? rtn->m_impl->spring_board->start() : target;
        const auto jump        = impl::make_jmp(start->addr(), destination, spring_board);

        if (!rtn->m_impl->source_page->protect(rwx))
        {
            return tl::make_unexpected(hook_error::protect);
        }

        if (!rtn->m_impl->source_page->protect(rwx))
        {
            return tl::make_unexpected(hook_error::protect);
        }

        rtn->m_impl->source->write(jump.data(), jump.size());
        rtn->m_impl->source_page->restore();

        return rtn;
    }

    std::size_t hook_base::impl::required_prologue_size(bool near) const
    {
        auto required = near ? size::jmp_near : size::jmp_far;
        auto rtn      = 0u;

        auto inst = instruction::unsafe(source->addr());

        while (rtn < required)
        {
            rtn += inst.size();
            inst = std::move(inst.next().value());
        }

        return rtn;
    }

    std::size_t hook_base::impl::estimate_trampoline_size(bool near) const
    {
        auto rtn        = prologue.size() + (near ? size::jmp_near : size::jmp_far);
        const auto addr = reinterpret_cast<std::uintptr_t>(prologue.data());

        for (auto i = 0u; prologue.size() > i;)
        {
            auto current = instruction::unsafe(addr + i);
            i += current.size();

            if (!current.relative())
            {
                continue;
            }

            if (!current.branching())
            {
                rtn += sizeof(std::uintptr_t);
                continue;
            }

            rtn += (near ? size::jmp_near : size::jmp_far);
        }

        return rtn;
    }

    bool hook_base::impl::create_springboard()
    {
        spring_board = page::allocate(source->addr(), size::jmp_far, rwx);

        if (!spring_board)
        {
            return false;
        }

        auto content = make_jmp(0, target, false);
        address::unsafe(spring_board->start()).write(content.data(), content.size());

        return true;
    }

    bool hook_base::impl::create_trampoline(bool near, bool spring_board)
    {
        prologue = source->copy(required_prologue_size(spring_board));

        const auto size = estimate_trampoline_size(near);
        trampoline.reset();

        if (near)
        {
            trampoline = page::allocate(source->addr(), size, rwx);
        }
        else
        {
            trampoline = page::allocate(size, rwx);
        }

        if (!trampoline)
        {
            return false;
        }

        auto content = prologue;
        content.reserve(size);

        const auto original = source->addr() + prologue.size();
        std::ranges::move(make_jmp(trampoline->start() + prologue.size(), original, near), std::back_inserter(content));

        const auto start = reinterpret_cast<std::uintptr_t>(content.data());

        for (auto i = 0u, inst_size = 0u; prologue.size() > i; i += inst_size)
        {
            auto inst = instruction::unsafe(start + i);
            inst_size = inst.size();

            if (!inst.relative())
            {
                continue;
            }

            const auto original_rip    = source->addr() + i;
            const auto original_target = inst.absolute(original_rip).value();

            const auto rip        = trampoline->start() + i;
            const auto new_offset = trampoline->start() + content.size() - rip - inst_size;

            auto offset = offset_of(inst);

            if (!offset)
            {
                return false;
            }

            if (try_offset(offset.value(), static_cast<std::intptr_t>(rip - original_rip)))
            {
                continue;
            }
            else if (!inst.branching())
            {
                std::ranges::move(make_ptr(original_target), std::back_inserter(content));
            }
            else
            {
                std::ranges::move(make_jmp(0, original_target, false), std::back_inserter(content));
            }

            if (try_redirect(offset.value(), static_cast<std::intptr_t>(new_offset)))
            {
                continue;
            }

            return false;
        }

        if (content.size() > size)
        {
            return false;
        }

        address::unsafe(trampoline->start()).write(content.data(), content.size());

        return true;
    }

    std::optional<offset_ptr> hook_base::impl::offset_of(const instruction &source)
    {
        const auto disp = source.displacement();

        if (auto offset = offset_of(source.addr(), disp); offset)
        {
            return offset;
        }

        if (auto offset = offset_of(source.addr(), source.immediates()); offset)
        {
            return offset;
        }

        return std::nullopt;
    }

    std::optional<offset_ptr> hook_base::impl::offset_of(std::uintptr_t source, const disp &disp)
    {
        const auto addr = source + disp.offset;

        switch (disp.size)
        {
        case 8:
            return reinterpret_cast<std::int8_t *>(addr);
        case 16:
            return reinterpret_cast<std::int16_t *>(addr);
        case 32:
            return reinterpret_cast<std::int32_t *>(addr);
        }

        return std::nullopt;
    }

    std::optional<offset_ptr> hook_base::impl::offset_of(std::uintptr_t source, const std::vector<imm> &immediates)
    {
        for (const auto &imm : immediates)
        {
            if (imm.size <= 0 || !imm.relative)
            {
                continue;
            }

            const auto is_signed = std::holds_alternative<std::int64_t>(imm.amount);
            const auto addr      = source + imm.offset;

            switch (imm.size)
            {
            case 8:
                return is_signed ? offset_ptr{reinterpret_cast<std::int8_t *>(addr)}
                                 : offset_ptr{reinterpret_cast<std::uint8_t *>(addr)};
            case 16:
                return is_signed ? offset_ptr{reinterpret_cast<std::int16_t *>(addr)}
                                 : offset_ptr{reinterpret_cast<std::uint16_t *>(addr)};
            case 32:
                return is_signed ? offset_ptr{reinterpret_cast<std::int32_t *>(addr)}
                                 : offset_ptr{reinterpret_cast<std::uint32_t *>(addr)};
            }
        }

        return std::nullopt;
    }

    bool hook_base::impl::try_offset(const offset_ptr &source, std::intptr_t offset)
    {
        auto visitor = [&offset]<typename T>(T *source)
        {
            auto desired = static_cast<std::intptr_t>(*source) - offset;
            auto actual  = *source - static_cast<T>(offset);

            if (static_cast<std::intptr_t>(actual) != desired)
            {
                return false;
            }

            *source = actual;

            return true;
        };

        return std::visit(visitor, source);
    }

    bool hook_base::impl::try_redirect(const offset_ptr &source, std::intptr_t target)
    {
        auto visitor = [&target]<typename T>(T *source)
        {
            if (std::numeric_limits<T>::max() < target)
            {
                return false;
            }

            if (std::numeric_limits<T>::min() > target)
            {
                return false;
            }

            *source = static_cast<T>(target);

            return true;
        };

        return std::visit(visitor, source);
    }

    std::vector<std::uint8_t> hook_base::impl::make_ptr(std::uintptr_t address)
    {
        std::vector<std::uint8_t> rtn(sizeof(std::uintptr_t));
        *reinterpret_cast<std::uintptr_t *>(rtn.data()) = address;

        return rtn;
    }

    std::vector<std::uint8_t> hook_base::impl::make_jmp(std::uintptr_t source, std::uintptr_t target, bool near)
    {
        std::vector<std::uint8_t> rtn;

        if (near || arch == architecture::x86)
        {
            rtn = {0xE9};
            rtn.resize(rtn.size() + sizeof(std::int32_t));

            auto relative = static_cast<std::int32_t>(target - source - size::jmp_near);
            *reinterpret_cast<std::int32_t *>(rtn.data() + 1) = relative;
        }
        else
        {
            rtn = {0xFF, 0x25, 0x00, 0x00, 0x00, 0x00};
            std::ranges::move(make_ptr(target), std::back_inserter(rtn));
        }

        return rtn;
    }
} // namespace lime
