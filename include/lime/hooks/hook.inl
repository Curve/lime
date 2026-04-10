#pragma once

#include "hook.hpp"

#include <thread>
#include <chrono>

namespace lime
{
    template <typename, typename>
    struct detail::is_callable : std::false_type
    {
    };

    template <typename T, typename R, typename... Ts>
        requires std::same_as<std::invoke_result_t<T, Ts...>, R>
    struct detail::is_callable<T, R(Ts...)> : std::true_type
    {
    };

    namespace detail
    {
        template <typename, typename>
        struct data;
    }

    template <typename Hook, typename T>
    struct detail::data
    {
        T callable;
        Hook hook;
    };

    template <typename R, typename... Ts, auto U>
    hook<R(Ts...), U>::hook(hook &&base) noexcept = default;

    template <typename R, typename... Ts, auto U>
    hook<R(Ts...), U>::hook(basic_hook &&base) noexcept : basic_hook(std::move(base))
    {
    }

    template <typename R, typename... Ts, auto U>
    hook<R(Ts...), U>::pointer hook<R(Ts...), U>::reset()
    {
        return reinterpret_cast<pointer>(basic_hook::reset());
    }

    template <typename R, typename... Ts, auto U>
    hook<R(Ts...), U>::pointer hook<R(Ts...), U>::original() const
    {
        return reinterpret_cast<pointer>(basic_hook::original());
    }

    template <typename R, typename... Ts, auto U>
    template <bool Wait, typename T, typename>
    basic_hook::res<hook<R(Ts...), U> *> hook<R(Ts...), U>::create(Address auto source, T &&target)
        requires Callable<T, R(hook &, Ts...)>
    {
        static auto data     = detail::data<hook *, T>{};
        static auto callback = [](Ts... params) -> R
        {
            if constexpr (Wait)
            {
                while (!data.hook)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }

            return data.callable(*data.hook, std::forward<Ts>(params)...);
        };

        auto rtn = create(source, static_cast<pointer>(&traits::template wrapper<callback>));

        if (!rtn.has_value())
        {
            return std::unexpected{rtn.error()};
        }

        data.callable = std::forward<T>(target);
        data.hook     = new hook{std::move(*rtn)};

        return data.hook;
    }

    template <typename R, typename... Ts, auto U>
    basic_hook::res<hook<R(Ts...), U>> hook<R(Ts...), U>::create(Address auto source, pointer target)
    {
        const auto source_address = reinterpret_cast<std::uintptr_t>(source);
        const auto target_address = reinterpret_cast<std::uintptr_t>(target);

        auto rtn = basic_hook::create(source_address, target_address);

        if (!rtn.has_value())
        {
            return std::unexpected{rtn.error()};
        }

        return hook{std::move(*rtn)};
    }

    template <typename R, typename... Ts, typename T>
    auto make_hook(R (*source)(Ts...), T &&replacement)
    {
        return hook<R(Ts...)>::create(source, std::forward<T>(replacement));
    }
} // namespace lime
