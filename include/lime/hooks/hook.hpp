#pragma once

#include "../utils/convention.hpp"

#include <memory>
#include <cstdint>
#include <expected>

#include <concepts>
#include <type_traits>

namespace lime
{
    using utils::calling_convention;

    namespace detail
    {
        template <typename, typename>
        struct is_callable;
    } // namespace detail

    template <typename T, typename Signature>
    concept Callable = detail::is_callable<T, Signature>::value;

    template <typename T>
    concept Address = std::convertible_to<T, std::uintptr_t> || std::is_pointer_v<T>;

    class basic_hook
    {
        struct impl;

      public:
        enum class error : std::uint8_t
        {
            not_unique,
            bad_page,
            bad_prot,
            bad_func,
            relocate,
            protect,
        };

        template <typename T>
        using res = std::expected<T, error>;

      private:
        std::unique_ptr<impl> m_impl;

      public:
        basic_hook(impl);

      public:
        basic_hook(basic_hook &&) noexcept;
        basic_hook &operator=(basic_hook &&) noexcept;

      public:
        ~basic_hook();

      protected:
        [[nodiscard]] std::uintptr_t reset() &&;
        [[nodiscard]] std::uintptr_t original() const;

      protected:
        static res<basic_hook> create(std::uintptr_t, std::uintptr_t);
    };

    template <typename, auto = calling_convention::cc_generic>
    struct hook;

    template <typename R, typename... Ts, auto U>
    struct hook<R(Ts...), U> : public basic_hook
    {
        using basic_hook::error;
        using basic_hook::res;

      public:
        using traits  = utils::convention_traits<R(Ts...), U>;
        using pointer = traits::pointer;

      public:
        hook(hook &&) noexcept;
        hook(basic_hook &&) noexcept;

      public:
        [[nodiscard]] pointer reset() &&;
        [[nodiscard]] pointer original() const;

      public:
        template <typename T, typename = decltype([] {})>
        static res<hook *> create(Address auto source, T &&target)
            requires Callable<T, R(hook &, Ts...)>;

      public:
        static res<hook> create(Address auto source, pointer target);
    };

    template <typename R, typename... Ts, typename T>
    auto make_hook(R (*source)(Ts...), T &&replacement);
} // namespace lime

#include "hook.inl"
