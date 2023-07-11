#include <catch2/catch_test_macros.hpp>
#include <lime/hooks/hook.hpp>

int test(int param)
{
    return param;
}

std::unique_ptr<lime::hook<int(int)>> original_test;
int test_hook(int param)
{
    return original_test->original()(param + 5);
}

TEST_CASE("Hooks are tested", "[hook]")
{
    REQUIRE(test(10) == 10);

    original_test = std::move(lime::hook<int(int)>::create(test, test_hook).value());
    REQUIRE(test(10) == 15);

    original_test.reset();
    REQUIRE(test(10) == 10);

    lime::hook<int(int)>::create(test, [](auto *hook, int param) -> int {
        auto rtn = hook->original()(param + 10);
        delete hook;
        return rtn;
    });

    REQUIRE(test(10) == 20);
    REQUIRE(test(10) == 10);
}