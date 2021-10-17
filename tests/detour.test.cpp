#include <catch2/catch.hpp>
#include <hooks/detour.hpp>

std::unique_ptr<lime::detour> int_rtn_detour;
int int_rtn_original(int param)
{
    return param + 1;
}
int int_rtn_hook(int param)
{
    return int_rtn_detour->get_original<int(int)>()(param + 1);
}
int int_rtn_hook2(int param)
{
    return param - 1;
}

TEST_CASE("Detours are tested", "[detours]")
{
    lime::detour_status status{};

    int_rtn_detour = lime::detour::create(reinterpret_cast<std::uintptr_t>(&int_rtn_original), reinterpret_cast<std::uintptr_t>(&int_rtn_hook), status);
    REQUIRE(status == lime::detour_status::success);

    REQUIRE(int_rtn_original(10) == 12);
    int_rtn_detour.reset();
    REQUIRE(int_rtn_original(10) == 11);

    int_rtn_detour = lime::detour::create(int_rtn_original, int_rtn_hook2, status);
    REQUIRE(status == lime::detour_status::success);

    REQUIRE(int_rtn_original(10) == 9);
    int_rtn_detour.reset();
    REQUIRE(int_rtn_original(10) == 11);
}