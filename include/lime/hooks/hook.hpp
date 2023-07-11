#pragma once
#include "../page.hpp"

#include <memory>
#include <cstdint>
#include <concepts>

#include <functional>
#include <tl/expected.hpp>
#include <boost/callable_traits.hpp>

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

    template <typename Hook, typename Signature>
    consteval auto lambda_target()
    {
        using args_t = boost::callable_traits::args_t<Signature>;
        using rtn_t = boost::callable_traits::return_type_t<Signature>;

        return std::apply(
            []<typename... T>(T &&...) {
                using func_t = std::function<rtn_t(Hook, T...)>;
                return std::type_identity<func_t>{};
            },
            args_t{});
    }

    template <typename Hook, typename Signature>
    using lambda_target_t = typename decltype(lambda_target<Hook, Signature>())::type;

    template <typename Signature>
    class hook : public hook_base
    {
        using signature_t = std::conditional_t<std::is_pointer_v<Signature>, Signature, Signature *>;
        using rtn_t = tl::expected<std::unique_ptr<hook>, hook_error>;

      public:
        signature_t original() const;

      public:
        template <Address Source, Address Target>
        [[nodiscard]] static rtn_t create(Source source, Target target);

      public:
        template <Address Source>
        static void create(Source source, lambda_target_t<hook *, Signature> &&target);
    };
} // namespace lime

#include "hook.inl"