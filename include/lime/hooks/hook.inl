#pragma once

#include "hook.hpp"

#include <bit>

namespace lime
{
    template <detail::function_signature Signature, convention Convention>
    typename hook<Signature, Convention>::signature_t hook<Signature, Convention>::original() const
    {
        return reinterpret_cast<signature_t>(hook_base::original());
    }

    template <detail::function_signature Signature, convention Convention>
    template <detail::address Source, detail::address Target>
    hook<Signature, Convention>::rtn_t<std::unique_ptr> hook<Signature, Convention>::create(Source source,
                                                                                            Target target)
    {
        auto source_address = std::bit_cast<std::uintptr_t>(source);
        auto target_address = std::bit_cast<std::uintptr_t>(target);

        auto rtn = hook_base::create(source_address, target_address);

        if (!rtn)
        {
            return tl::make_unexpected(rtn.error());
        }

        auto *ptr = rtn->release();
        return std::unique_ptr<hook>{static_cast<hook *>(ptr)};
    }

    template <detail::function_signature Signature, convention Convention>
    template <detail::address Source, detail::lambda_like Callable>
    hook<Signature, Convention>::rtn_t<std::add_pointer_t> hook<Signature, Convention>::create(Source source,
                                                                                               Callable &&target)
    {
        static hook<Signature, Convention> *rtn;
        [[maybe_unused]] static auto lambda = std::forward<Callable>(target);

        static constexpr auto dispatch = []<typename... T>(T &&...args)
        {
            return lambda(rtn, std::forward<T>(args)...);
        };

        auto wrapper = detail::calling_convention<Signature, Convention>::template wrapper<dispatch>;
        auto result  = create(source, wrapper);

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
