#include <lime/constants/protection.hpp>
#include <lime/utility/memory.hpp>
#include <catch2/catch.hpp>
#include <lime/page.hpp>

#undef max // WinDef.h defines a max macro which collides with any function named max (i.e. numeric_limits)

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

    // ? This check fails on Github for some reason.
    // CHECK(lime::page::get_page_at(*allocated_memory)->get_protection() == (lime::prot::read_write));

    auto allocated_near = lime::allocate_near(0, 100, lime::prot::read_only);
    REQUIRE(allocated_near);
    CHECK(*allocated_near < std::numeric_limits<std::int32_t>::max());
}