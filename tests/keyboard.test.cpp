#include <lime/constants/key_codes.hpp>
#include <lime/utility/keyboard.hpp>
#include <catch2/catch.hpp>

TEST_CASE("Keyboard utility is tested", "[keyboard]")
{
    lime::keyboard::release(lime::key::escape);
    REQUIRE(!lime::keyboard::is_down(lime::key::escape));

    lime::keyboard::press(lime::key::escape);
    REQUIRE(lime::keyboard::was_pressed(lime::key::escape));
    REQUIRE(!lime::keyboard::was_pressed(lime::key::escape));
    REQUIRE(lime::keyboard::is_down(lime::key::escape));

    lime::keyboard::release(lime::key::escape);
    REQUIRE(!lime::keyboard::is_down(lime::key::escape));
}