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

suite<"Hooks"> hook_suite = []
{
    expect(eq(test_fn(10), 10));
    expect(eq(test_fn(1337), 0));

    original_test.emplace(*lime::make_hook(test_fn, test_hook));
    expect(eq(test_fn(10), 15));
    expect(eq(test_fn(1337), 1342));

    original_test.reset();
    expect(eq(test_fn(10), 10));

    original_test.emplace(*lime::make_hook(test_fn, test_hook));
    expect(eq(test_fn(10), 15));

    original_test.reset();
    expect(eq(test_fn(10), 10));

    lime::make_hook(test_fn, [](auto &hook, int param) { return hook.reset()(param + 5); });

    expect(eq(test_fn(10), 15));
    expect(eq(test_fn(10), 10));

    lime::make_hook(test_fn, [](auto &hook, int param) { return hook.reset()(param + 10); });

    expect(eq(test_fn(10), 20));
    expect(eq(test_fn(10), 10));
};
