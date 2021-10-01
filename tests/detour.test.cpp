#define CONFIG_CATCH_MAIN
#include <catch2/catch.hpp>
#include <hooks/detour.hpp>

#ifdef __GNUC__
#define NOOPT __attribute__((optimize("-O0")))
#else
#define NOOPT
#endif

#ifdef _WIN32
#pragma optimize("", off)
#endif

std::unique_ptr<lime::detour> int_rtn_detour;
int NOOPT int_rtn_original(int param)
{
    return param + 1;
}
int NOOPT int_rtn_hook(int param)
{
    return int_rtn_detour->get_original<int(int)>()(param + 1);
}
int NOOPT int_rtn_hook2(int param)
{
    return param - 1;
}

#ifdef _WIN32
#pragma optimize("", on)
#endif

TEST_CASE("Detours are tested", "[detours]")
{
    int_rtn_detour = lime::detour::create(int_rtn_original, int_rtn_hook);

    REQUIRE(int_rtn_original(10) == 12);
    int_rtn_detour.reset();
    REQUIRE(int_rtn_original(10) == 11);

    int_rtn_detour = lime::detour::create(int_rtn_original, int_rtn_hook2);
    REQUIRE(int_rtn_original(10) == 9);
    int_rtn_detour.reset();
    REQUIRE(int_rtn_original(10) == 11);
}