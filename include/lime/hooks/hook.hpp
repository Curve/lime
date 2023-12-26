#pragma once

#include <memory>
#include <cstdint>
#include <concepts>

#include <tl/expected.hpp>

namespace lime
{
    enum class hook_error
    {
        protect,
        relocate,
        bad_page,
    };

    class hook_base
    {
        struct impl;

      protected:
        using rtn_t = tl::expected<std::unique_ptr<hook_base>, hook_error>;

      protected:
        std::unique_ptr<impl> m_impl;

      protected:
        hook_base();

      public:
        ~hook_base();

      public:
        hook_base(hook_base &&) noexcept;

      protected:
        [[nodiscard]] std::uintptr_t original() const;

      protected:
        [[nodiscard]] static rtn_t create(std::uintptr_t, std::uintptr_t);
    };

    template <typename T>
    concept Address = requires() { requires std::integral<T> || std::is_pointer_v<T>; };

    template <typename T>
    concept PointerDecay = requires(T callable) {
        { +callable };
    };

    template <typename T, typename Signature>
    consteval auto can_invoke();

    template <typename T, typename Signature>
    concept Lambda = requires(T callable) {
        requires not(Address<T> or PointerDecay<T>);
        requires can_invoke<T, Signature>();
    };

    template <typename Signature>
    class hook : public hook_base
    {
        template <template <typename...> typename T>
        using rtn_t       = tl::expected<T<hook>, hook_error>;
        using signature_t = std::conditional_t<std::is_pointer_v<Signature>, Signature, Signature *>;

      public:
        signature_t original() const;

      public:
        [[nodiscard]] static rtn_t<std::unique_ptr> create(Address auto source, Address auto target);

      public:
        template <typename Callable>
            requires Lambda<Callable, Signature>
        static rtn_t<std::add_pointer_t> create(Address auto source, Callable &&target);
    };
} // namespace lime

#include "hook.inl"
