#pragma once

#include "convention.hpp"

#include <memory>
#include <cstdint>
#include <concepts>
#include <functional>

#include <tl/expected.hpp>

namespace lime
{
    namespace detail
    {
        template <typename T>
        concept is_std_function = requires(T value) {
            []<typename R>(std::function<R> &) {
            }(value);
        };

        template <typename T>
        concept function_signature = requires() {
            requires not std::is_pointer_v<T>;
            requires std::is_function_v<T>;
        };

        template <typename T>
        concept function_pointer = requires() {
            requires std::is_pointer_v<T> or std::is_reference_v<T>;
            requires std::is_function_v<std::remove_pointer_t<std::remove_reference_t<T>>>;
        };

        template <typename T>
        concept address = requires() { requires std::integral<T> or function_pointer<T>; };

        template <typename T>
        concept lambda_like = requires() {
            requires not address<T>;
            requires not is_std_function<T>;
        };
    } // namespace detail

    enum class hook_error
    {
        protect,
        relocate,
        bad_page,
        bad_prot,
        bad_func,
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

    template <detail::function_signature Signature, convention Convention = convention::automatic>
    class hook : public hook_base
    {
        template <template <typename...> typename T>
        using rtn_t       = tl::expected<T<hook>, hook_error>;
        using signature_t = std::add_pointer_t<typename detail::calling_convention<Signature, Convention>::add>;

      public:
        signature_t original() const;

      public:
        template <detail::address Source, detail::address Target>
        [[nodiscard]] static rtn_t<std::unique_ptr> create(Source source, Target target);

      public:
        template <detail::address Source, detail::lambda_like Callable>
        static rtn_t<std::add_pointer_t> create(Source source, Callable &&target);
    };

    template <detail::function_pointer Signature, convention Convention = convention::automatic, typename Callable>
    auto make_hook(Signature source, Callable &&target);

    template <typename Signature, convention Convention = convention::automatic, typename Callable>
    auto make_hook(detail::address auto source, Callable &&target);
} // namespace lime

#include "hook.inl"
