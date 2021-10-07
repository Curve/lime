#include <catch2/catch.hpp>
#include <fstream>
#include <iostream>
#include <utility/console.hpp>

TEST_CASE("Console utility is tested", "[console]")
{
    lime::console::redirect_to_file("test.txt");
    auto test_file = std::ifstream("test.txt");

    REQUIRE(test_file);

    test_file.seekg(0, std::ios::end);
    CHECK(test_file.tellg() == 0);

    std::cout << "Test" << std::endl;

    test_file.seekg(0, std::ios::end);
    CHECK(test_file.tellg() != 0);

    lime::console::restore();
    test_file.close();

#if defined(__linux__)
    REQUIRE(!std::filesystem::exists("/tmp/lime_test"));

    lime::console::alloc_console("test");

    auto tmp_file = std::ifstream("/tmp/lime_test");
    REQUIRE(tmp_file);

    tmp_file.seekg(0, std::ios::end);
    CHECK(tmp_file.tellg() == 0);

    std::cout << "Test" << std::endl;

    tmp_file.seekg(0, std::ios::end);
    CHECK(tmp_file.tellg() != 0);

    lime::console::restore();
    tmp_file.close();
#endif
}