#pragma once
#include "hook.hpp"

namespace lime
{
    template <typename Signature>
    typename hook<Signature>::signature_t hook<Signature>::original() const
    {
        return reinterpret_cast<signature_t>(hook_base::original());
    }

    template <typename Signature>
    template <Address Source, Address Target>
    typename hook<Signature>::rtn_t hook<Signature>::create(Source source, Target target)
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
    void hook<Signature>::create(Source source, lambda_target_t<hook *, Signature> &&target)
    {
        using args_t = boost::callable_traits::args_t<Signature>;
        using rtn_t = boost::callable_traits::return_type_t<Signature>;

        static hook<Signature> *hook;
        static auto lambda = std::move(target);

        static constexpr auto wrapper = std::apply(
            []<typename... T>(T &&...) consteval {
                return [](T... args) -> rtn_t {
                    return lambda(hook, std::forward<T>(args)...);
                };
            },
            args_t{});

        auto _hook = create(source, +wrapper);
        assert(((void)"Failed to create hook", _hook));

        hook = _hook->release();
    }
} // namespace lime