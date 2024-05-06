#include <lime/hooks/hook.hpp>
#include <boost/ut.hpp>

using namespace boost::ut;
using namespace boost::ut::literals;

int test_fn(int param)
{
    return param;
}

std::unique_ptr<lime::hook<int(int)>> original_test;

int test_hook(int param)
{
    return original_test->original()(param + 5);
}

suite<"Hooks"> hook_suite = []
{
    expect(eq(test_fn(10), 10));

    original_test = std::move(lime::hook<int(int)>::create(test_fn, test_hook).value());
    expect(eq(test_fn(10), 15));

    original_test.reset();
    expect(eq(test_fn(10), 10));

    original_test = std::move(lime::make_hook(test_fn, test_hook).value());
    expect(eq(test_fn(10), 15));

    original_test.reset();
    expect(eq(test_fn(10), 10));

    lime::hook<int(int)>::create(test_fn,
                                 [](auto *hook, int param) -> int
                                 {
                                     auto rtn = hook->original()(param + 10);
                                     delete hook;
                                     return rtn;
                                 });

#if INTPTR_MAX == INT32_MAX
    using hook_t = lime::hook<int(void *, int), lime::convention::c_fastcall>;

    hook_t::create(0xDEADBEEF,
                   [&](auto *hook, void *thiz, int param) -> int
                   {
                       auto ret = hook->original()(thiz, param);
                       delete hook;
                       return ret;
                   });
#endif

    expect(eq(test_fn(10), 20));
    expect(eq(test_fn(10), 10));

    lime::make_hook(test_fn,
                    [](auto *hook, int param) -> int
                    {
                        auto rtn = hook->original()(param + 10);
                        delete hook;
                        return rtn;
                    });

    expect(eq(test_fn(10), 20));
    expect(eq(test_fn(10), 10));
};
