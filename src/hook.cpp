#include "hooks/hook.hpp"

#include "page.hpp"
#include "address.hpp"
#include "constants.hpp"
#include "instruction.hpp"

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

    struct basic_hook::impl
    {
        address source;
        page source_page;

      public:
        std::uintptr_t target;
        std::vector<std::uint8_t> prologue;

      public:
        std::optional<page> trampoline;
        std::optional<page> spring_board;

      public:
        [[nodiscard]] std::size_t required_prologue_size(bool near) const;
        [[nodiscard]] std::size_t estimate_trampoline_size(bool near) const;

      public:
        bool create_springboard();
        bool create_trampoline(bool near, bool spring_board);

      public:
        static std::optional<offset_ptr> offset_of(const instruction &source);
        static std::optional<offset_ptr> offset_of(std::uintptr_t, const disp &);
        static std::optional<offset_ptr> offset_of(std::uintptr_t, const std::array<imm, 2> &);

      public:
        static bool try_offset(const offset_ptr &source, std::intptr_t offset);
        static bool try_redirect(const offset_ptr &source, std::intptr_t target);

      public:
        static std::vector<std::uint8_t> make_ptr(std::uintptr_t address);
        static std::vector<std::uint8_t> make_jmp(std::uintptr_t source, std::uintptr_t target, bool near = false);
    };

    basic_hook::basic_hook(impl &&data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    basic_hook::basic_hook(basic_hook &&) noexcept = default;

    basic_hook &basic_hook::operator=(basic_hook &&) noexcept = default;

    basic_hook::~basic_hook()
    {
        if (!m_impl)
        {
            return;
        }

        std::ignore = reset();
    }

    std::uintptr_t basic_hook::reset()
    {
        if (!m_impl || !m_impl->source_page.protect(rwx))
        {
            assert(false && "Failed to protect original function for restoration");
            return {};
        }

        m_impl->source.write({m_impl->prologue});
        m_impl->source_page.restore();

        const auto rtn = m_impl->source.value();
        m_impl.reset();

        return rtn;
    }

    std::uintptr_t basic_hook::original() const
    {
        return m_impl->trampoline->start();
    }

    std::expected<basic_hook, basic_hook::error> basic_hook::create(std::uintptr_t source, std::uintptr_t target)
    {
        auto page = page::at(source);

        if (!page.has_value())
        {
            return std::unexpected{error::bad_page};
        }

        if (!page->can(protection::read))
        {
            return std::unexpected{error::bad_prot};
        }

        auto start = instruction::at(source);

        if (!start.has_value())
        {
            return std::unexpected{error::bad_func};
        }

        if (auto follow = start->follow().transform([](auto addr) { return instruction::at(addr); }); follow && follow->has_value())
        {
            start = std::move(**follow);
            page.emplace(page::at(start->address()).value());
        }

        auto rtn = impl{
            .source      = address::unsafe(start->address()),
            .source_page = std::move(*page),
            .target      = target,
        };

        const auto spring_board = rtn.create_springboard();
        const auto near         = rtn.create_trampoline(true, spring_board);

        if (!near && !rtn.create_trampoline(false, spring_board))
        {
            return std::unexpected{error::relocate};
        }

        const auto destination = spring_board ? rtn.spring_board->start() : target;

        auto jump            = impl::make_jmp(start->address(), destination, spring_board);
        const auto remaining = rtn.prologue.size() - jump.size();

        if (remaining > 0)
        {
            std::vector<std::uint8_t> padding(remaining, 0x90);
            std::ranges::move(padding, std::back_inserter(jump));
        }

        if (!rtn.source_page.protect(rwx))
        {
            return std::unexpected{error::protect};
        }

        rtn.source.write({jump});
        rtn.source_page.restore();

        return std::move(rtn);
    }

    std::size_t basic_hook::impl::required_prologue_size(bool near) const
    {
        const auto required = near ? size::jmp_near : size::jmp_far;

        auto rtn  = 0u;
        auto inst = instruction::at(source.value()).value();

        while (rtn < required)
        {
            rtn += inst.size();
            inst = std::move(inst.next().value());
        }

        return rtn;
    }

    std::size_t basic_hook::impl::estimate_trampoline_size(bool near) const
    {
        auto rtn        = prologue.size() + (near ? size::jmp_near : size::jmp_far);
        const auto addr = reinterpret_cast<std::uintptr_t>(prologue.data());

        for (auto i = 0u; prologue.size() > i;)
        {
            auto current = instruction::at(addr + i).value();
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

    bool basic_hook::impl::create_springboard()
    {
        spring_board = page::allocate(source.value(), size::jmp_far, rwx);

        if (!spring_board)
        {
            return false;
        }

        auto content = make_jmp(spring_board->start(), target, false);
        address::unsafe(spring_board->start()).write({content});

        return true;
    }

    bool basic_hook::impl::create_trampoline(bool near, bool spring_board)
    {
        prologue = source.copy(required_prologue_size(spring_board));

        const auto size = estimate_trampoline_size(near);
        trampoline.reset();

        if (near)
        {
            trampoline = page::allocate(source.value(), size, rwx);
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

        const auto original = source.value() + prologue.size();
        std::ranges::move(make_jmp(trampoline->start() + prologue.size(), original, near), std::back_inserter(content));

        const auto start = reinterpret_cast<std::uintptr_t>(content.data());

        for (auto i = 0u, inst_size = 0u; prologue.size() > i; i += inst_size)
        {
            auto inst = instruction::at(start + i).value();
            inst_size = inst.size();

            if (!inst.relative())
            {
                continue;
            }

            const auto original_rip    = source.value() + i;
            const auto original_target = inst.follow(original_rip).value();

            const auto rip           = trampoline->start() + i;
            const auto reloc_address = trampoline->start() + content.size();

            const auto new_offset = reloc_address - rip - inst_size;
            auto offset           = offset_of(inst);

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
                std::ranges::move(make_jmp(reloc_address, original_target, false), std::back_inserter(content));
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

        address::unsafe(trampoline->start()).write({content});

        return true;
    }

    std::optional<offset_ptr> basic_hook::impl::offset_of(const instruction &source)
    {
        const auto disp = source.displacement();

        if (auto offset = offset_of(source.address(), disp); offset)
        {
            return offset;
        }

        if (auto offset = offset_of(source.address(), source.immediates()); offset)
        {
            return offset;
        }

        return std::nullopt;
    }

    std::optional<offset_ptr> basic_hook::impl::offset_of(std::uintptr_t source, const disp &disp)
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

    std::optional<offset_ptr> basic_hook::impl::offset_of(std::uintptr_t source, const std::array<imm, 2> &immediates)
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
                return is_signed ? offset_ptr{reinterpret_cast<std::int8_t *>(addr)} : offset_ptr{reinterpret_cast<std::uint8_t *>(addr)};
            case 16:
                return is_signed ? offset_ptr{reinterpret_cast<std::int16_t *>(addr)} : offset_ptr{reinterpret_cast<std::uint16_t *>(addr)};
            case 32:
                return is_signed ? offset_ptr{reinterpret_cast<std::int32_t *>(addr)} : offset_ptr{reinterpret_cast<std::uint32_t *>(addr)};
            }
        }

        return std::nullopt;
    }

    bool basic_hook::impl::try_offset(const offset_ptr &source, std::intptr_t offset)
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

    bool basic_hook::impl::try_redirect(const offset_ptr &source, std::intptr_t target)
    {
        auto visitor = [&target]<typename T>(T *source)
        {
            static constexpr auto max = static_cast<std::intptr_t>(std::numeric_limits<T>::max());
            static constexpr auto min = static_cast<std::intptr_t>(std::numeric_limits<T>::min());

            if (max < target)
            {
                return false;
            }

            if (min > target)
            {
                return false;
            }

            *source = static_cast<T>(target);

            return true;
        };

        return std::visit(visitor, source);
    }

    std::vector<std::uint8_t> basic_hook::impl::make_ptr(std::uintptr_t address)
    {
        std::vector<std::uint8_t> rtn(sizeof(std::uintptr_t));
        *reinterpret_cast<std::uintptr_t *>(rtn.data()) = address;

        return rtn;
    }

    std::vector<std::uint8_t> basic_hook::impl::make_jmp(std::uintptr_t source, std::uintptr_t target, bool near)
    {
        std::vector<std::uint8_t> rtn;

        if (near || arch == architecture::x86)
        {
            rtn = {0xE9};
            rtn.resize(rtn.size() + sizeof(std::int32_t));

            auto relative                                     = static_cast<std::int32_t>(target - source - size::jmp_near);
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
