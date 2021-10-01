#define CONFIG_CATCH_MAIN
#include <catch2/catch.hpp>
#include <constants/protection.hpp>
#include <page.hpp>

#ifdef __GNUC__
#define NOINLINE __attribute__((noinline))
#else
#define NOOPT __declspec(noinline)
#endif

int some_func()
{
    return 1;
}

int test_val = 0;

TEST_CASE("Pages are tested", "[pages]")
{
    REQUIRE_FALSE(lime::page::get_pages().empty());

    auto some_func_page = lime::page::get_page_at(reinterpret_cast<std::uintptr_t>(some_func));
    CHECK(some_func_page->get_protection() == (lime::prot::read_execute));

    auto test_val_page = lime::page::get_page_at(reinterpret_cast<std::uintptr_t>(&test_val));
    CHECK(test_val_page->get_protection() == (lime::prot::read_write));
}