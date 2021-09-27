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

    auto allocated_memory = lime::allocate(100, lime::prot_read);

    REQUIRE(allocated_memory);
    CHECK(lime::page::get_page_at(*allocated_memory)->get_protection() == lime::prot_read);

    CHECK(lime::protect(*allocated_memory, 100, lime::prot_read | lime::prot_write));
    CHECK(lime::page::get_page_at(*allocated_memory)->get_protection() == (lime::prot_read | lime::prot_write));

    auto previous_allocated = *allocated_memory;
    allocated_memory.reset();
    CHECK_FALSE(lime::page::get_page_at(previous_allocated));

    auto at_previous = lime::allocate_at(previous_allocated, 100, lime::prot_read);
    REQUIRE(at_previous);
    CHECK(lime::page::get_page_at(previous_allocated)->get_protection() == lime::prot_read);

    auto allocated_near = lime::allocate_near(reinterpret_cast<std::uintptr_t>(&val), 100, lime::prot_read);
    REQUIRE(allocated_near);
    CHECK(*allocated_near - reinterpret_cast<std::uintptr_t>(&val) < INT32_MAX);
}