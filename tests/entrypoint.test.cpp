#include <catch2/catch.hpp>
#include <utility/entrypoint.hpp>

template <typename... T> void entry(const T &...) {}
template <typename... T> void exit(const T &...) {}

TEST_CASE("Entrypoint code is tested", "[entrypoint]")
{
    //? This check just exists to check wether or not the entrypoint code compiles as we can't really test it here.
}