#include <lime/constants/os.hpp>
#include <catch2/catch.hpp>

TEST_CASE("OS constants are tested", "[os]")
{
#if defined(_WIN32)
    REQUIRE(lime::os == lime::operating_system::Windows);
#elif defined(__linux__)
    REQUIRE(lime::os == lime::operating_system::Linux);
#endif
}