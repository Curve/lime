#define CONFIG_CATCH_MAIN
#include <catch2/catch.hpp>
#include <constants/protection.hpp>
#include <page.hpp>
#include <utility/memory.hpp>

int val = 0;

TEST_CASE("Memory utils are tested", "[memory]")
{
    CHECK(lime::write(reinterpret_cast<std::uintptr_t>(&val), 100));
    CHECK(lime::read<int>(reinterpret_cast<std::uintptr_t>(&val)) == 100);
    REQUIRE(val == 100);

    auto allocated_memory = lime::allocate(100, lime::prot::read_only);

    REQUIRE(allocated_memory);
    CHECK(lime::page::get_page_at(*allocated_memory)->get_protection() == lime::prot::read_only);

    CHECK(lime::protect(*allocated_memory, 100, lime::prot::read_write));

    CHECK(lime::page::get_page_at(*allocated_memory)->get_protection() == (lime::prot::read_write));

    // auto previous_allocated = *allocated_memory;
    // allocated_memory.reset();

    // auto at_previous = lime::allocate_at(previous_allocated, 100, lime::prot::read_only);
    // REQUIRE(at_previous);
    // CHECK(lime::page::get_page_at(previous_allocated)->get_protection() == lime::prot::read_only);

    auto allocated_near = lime::allocate_near(0, 100, lime::prot::read_only);
    REQUIRE(allocated_near);
    CHECK(*allocated_near < INT32_MAX);
}