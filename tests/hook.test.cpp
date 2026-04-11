#include <boost/ut.hpp>

#include <print>
#include <lime/hooks/hook.hpp>

using namespace boost::ut;
using namespace boost::ut::literals;

int test_fn(int param)
{
    std::println("test_fn({}): called", param);

    if (param == 1337)
    {
        return 0;
    }

    return param;
}

std::optional<lime::hook<int(int)>> original_test;

int test_hook(int param)
{
    return original_test->original()(param + 5);
}

#ifdef __GNUC__
template <typename T>
inline void dont_optimize(T const &value)
{
    asm volatile("" : : "g"(value) : "memory");
}
#endif

suite<"Hooks"> hook_suite = []
{
    using test_fn_t                 = int (*)(int);
    volatile auto avoidOptimization = true;
    auto func                       = test_fn_t{};

    if (avoidOptimization)
    {
        func = test_fn;
    }
    else
    {
        func = reinterpret_cast<test_fn_t>(0xDEADBEEF);
    }

#ifdef __GNUC__
    dont_optimize(func);
#endif

    expect(eq(func(10), 10));
    expect(eq(func(1337), 0));

    if (auto hook = lime::make_hook(func, test_hook); hook.has_value())
    {
        original_test.emplace(*std::move(hook));
    }
    else
    {
        expect(false);
    }

    expect(eq(func(10), 15));
    expect(eq(func(1337), 1342));

    original_test.reset();
    expect(eq(func(10), 10));

    if (auto hook = lime::make_hook(func, test_hook); hook.has_value())
    {
        original_test.emplace(*std::move(hook));
    }
    else
    {
        expect(false);
    }

    expect(eq(func(10), 15));
    original_test.reset();
    expect(eq(func(10), 10));

    expect(lime::make_hook(func, [](auto &hook, int param) { return std::move(hook).reset()(param + 5); }).has_value());

    expect(eq(func(10), 15));
    expect(eq(func(10), 10));

    expect(lime::make_hook(func, [](auto &hook, int param) { return std::move(hook).reset()(param + 10); }).has_value());

    expect(eq(func(10), 20));
    expect(eq(func(10), 10));
};
