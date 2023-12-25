#pragma once
#include "hook.hpp"

#include <cassert>
#include <functional>

#include <boost/callable_traits.hpp>

namespace lime
{
    template <typename Hook, typename Signature>
    consteval auto lambda_target()
    {
        using args_t   = boost::callable_traits::args_t<Signature>;
        using return_t = boost::callable_traits::return_type_t<Signature>;

        return std::apply(
            []<typename... T>(T &&...)
            {
                using function_t = std::function<return_t(Hook, T...)>;
                return std::type_identity<function_t>{};
            },
            args_t{});
    }

    template <typename Signature>
    typename hook<Signature>::signature_t hook<Signature>::original() const
    {
        return reinterpret_cast<signature_t>(hook_base::original());
    }

    template <typename Signature>
    template <Address Source, Address Target>
    hook<Signature>::template rtn_t<std::unique_ptr> hook<Signature>::create(Source source, Target target)
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
    template <Address Source>
    hook<Signature>::template rtn_t<std::add_pointer_t>
    hook<Signature>::create(Source source, lambda_target_t<hook *, Signature> &&target)
    {
        using args_t = boost::callable_traits::args_t<Signature>;
        using rtn_t  = boost::callable_traits::return_type_t<Signature>;

        static hook<Signature> *hook;
        static auto lambda = std::move(target);

        static constexpr auto wrapper = std::apply(
            []<typename... T>(T &&...)
            {
                return [](T... args) -> rtn_t
                {
                    return lambda(hook, std::forward<T>(args)...);
                };
            },
            args_t{});

        auto result = create(source, static_cast<signature_t>(wrapper));

        if (!result)
        {
            return tl::make_unexpected(result.error());
        }

        hook = result->release();

        return hook;
    }
} // namespace lime
