#pragma once

#include "hook.hpp"

#include <cassert>
#include <boost/callable_traits.hpp>

namespace lime
{
    template <typename Lambda, typename Signature>
    consteval auto detail::can_invoke()
    {
        using args_t = boost::callable_traits::args_t<Signature>;
        using rtn_t  = boost::callable_traits::return_type_t<Signature>;

        return std::apply(
            []<typename... T>(T &&...)
            {
                if constexpr (!std::invocable<Lambda, hook<Signature> *, T...>)
                {
                    return false;
                }
                else
                {
                    return std::is_same_v<std::invoke_result_t<Lambda, hook<Signature> *, T...>, rtn_t>;
                }
            },
            args_t{});
    }

    template <typename Signature>
    typename hook<Signature>::signature_t hook<Signature>::original() const
    {
        return reinterpret_cast<signature_t>(hook_base::original());
    }

    template <typename Signature>
    template <detail::address Source, detail::address Target>
    hook<Signature>::rtn_t<std::unique_ptr> hook<Signature>::create(Source source, Target target)
    {
        auto source_address = reinterpret_cast<std::uintptr_t>(source);
        auto target_address = reinterpret_cast<std::uintptr_t>(target);

        auto rtn = hook_base::create(source_address, target_address);

        if (!rtn)
        {
            return tl::make_unexpected(rtn.error());
        }

        auto *ptr = rtn->release();
        return std::unique_ptr<hook>{static_cast<hook *>(ptr)};
    }

    template <typename Signature>
    template <typename Callable>
    requires detail::invocable_lambda_like<Callable, Signature>
    hook<Signature>::rtn_t<std::add_pointer_t> hook<Signature>::create(detail::address auto source, Callable &&target)
    {
        using args_t = boost::callable_traits::args_t<Signature>;
        using rtn_t  = boost::callable_traits::return_type_t<Signature>;

        static hook<Signature> *rtn;
        static auto lambda = std::forward<Callable>(target);

        static constexpr auto wrapper = std::apply(
            []<typename... T>(T &&...)
            {
                return [](T... args) -> rtn_t
                {
                    return lambda(rtn, std::forward<T>(args)...);
                };
            },
            args_t{});

        auto result = create(source, +wrapper);

        if (!result)
        {
            return tl::make_unexpected(result.error());
        }

        rtn = result->release();

        return rtn;
    }

    template <detail::function_pointer Signature, typename Callable>
    auto make_hook(Signature source, Callable &&target)
    {
        return hook<std::remove_pointer_t<Signature>>::create(source, std::forward<Callable>(target));
    }

    template <typename Signature, typename Callable>
    auto make_hook(detail::address auto source, Callable &&target)
    {
        return hook<Signature>::create(source, std::forward<Callable>(target));
    }
} // namespace lime
