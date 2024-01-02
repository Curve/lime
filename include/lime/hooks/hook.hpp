#pragma once

#include <memory>
#include <cstdint>
#include <concepts>

#include <tl/expected.hpp>

namespace lime
{
    namespace concepts
    {
        template <typename T>
        concept address = requires() { requires std::integral<T> or std::is_pointer_v<T>; };

        template <typename T>
        concept function_pointer = requires() //
        {
            requires std::is_pointer_v<T>;
            requires std::is_function_v<std::remove_pointer_t<T>>;
        };

        template <typename T>
        concept is_function = requires(T value) //
        {
            []<typename F>(std::function<F> &) {
            }(value);
        };

        template <typename T>
        concept lambda_like = requires() //
        {
            requires not address<T>;
            requires not is_function<T>;
            requires not function_pointer<T>;
            requires not std::assignable_from<T, T>;
        };

        template <typename T, typename Signature>
        consteval auto can_invoke();

        template <typename T, typename Signature>
        concept invocable_lambda_like = requires() { requires lambda_like<T> and can_invoke<T, Signature>(); };
    } // namespace concepts

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

    template <typename Signature>
    class hook : public hook_base
    {
        template <template <typename...> typename T>
        using rtn_t       = tl::expected<T<hook>, hook_error>;
        using signature_t = std::conditional_t<std::is_pointer_v<Signature>, Signature, Signature *>;

      public:
        signature_t original() const;

      public:
        template <concepts::address Source, concepts::address Target>
        [[nodiscard]] static rtn_t<std::unique_ptr> create(Source source, Target target);

      public:
        template <typename Callable>
        requires concepts::invocable_lambda_like<Callable, Signature>
        static rtn_t<std::add_pointer_t> create(concepts::address auto source, Callable &&target);
    };

    template <concepts::function_pointer Signature, typename Callable>
    auto make_hook(Signature source, Callable &&target);

    template <typename Signature, typename Callable>
    auto make_hook(concepts::address auto source, Callable &&target);
} // namespace lime

#include "hook.inl"
