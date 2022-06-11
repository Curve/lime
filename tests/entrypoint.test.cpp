#include <lime/utility/entrypoint.hpp>
#include <catch2/catch.hpp>

inline bool success = false;

template <typename... T> void entry(const T &...)
{
    success = true;
}

template <typename... T> void quit(const T &...) {}

TEST_CASE("Entrypoint code is tested", "[entrypoint]")
{
//? This check just exists to check wether or not the entrypoint code compiles as we can't really test it here.
#if defined(__linux__)
    REQUIRE(success);
#endif
}