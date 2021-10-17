#include <catch2/catch.hpp>
#include <module.hpp>

#if defined(__linux__)
#define EXPORT __attribute__((visibility("default")))
#elif defined(_WIN32)
#define EXPORT __declspec(dllexport)
#endif

extern "C" EXPORT void exported_func()
{
    printf("Hello from exported function!\n");
}

TEST_CASE("Modules are tested", "[modules]")
{
    REQUIRE_FALSE(lime::module::get_modules().empty());

    auto this_module = lime::module::get("");
    REQUIRE(this_module);

    auto exported_fun = this_module->get_symbol("exported_func");
    CHECK(exported_fun == reinterpret_cast<std::uintptr_t>(exported_func));

    REQUIRE(!this_module->get_symbols().empty());

    auto symbol = this_module->find_symbol("exported_func");
    CHECK(symbol);
    CHECK(*symbol == exported_fun);
}