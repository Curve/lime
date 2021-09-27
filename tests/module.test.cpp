#define CONFIG_CATCH_MAIN
#include <catch2/catch.hpp>
#include <module.hpp>

extern "C" __attribute__((visibility("default"))) void exported_func()
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
}