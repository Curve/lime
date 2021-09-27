#define CONFIG_CATCH_MAIN
#include <catch2/catch.hpp>
#include <utility/signature.hpp>

void test()
{
    __asm("mov $1337, %rax\n"
          "sub $917, %rax");
}

TEST_CASE("Signature scanning is tested", "[signature]")
{
    auto test_page = lime::page::get_page_at(reinterpret_cast<std::uintptr_t>(test));
    auto this_module = lime::module::get("");

    REQUIRE(test_page);
    REQUIRE(this_module);

    auto ida_sig = lime::sig("?? C7 ? 39 05 00 00 48 2D 95 03 00 00");
    CHECK((*ida_sig.find<false>() - reinterpret_cast<std::uintptr_t>(test)) < 10);
    CHECK((ida_sig.find<true>().front() - reinterpret_cast<std::uintptr_t>(test)) < 10);

    CHECK((*ida_sig.find_in<false>(*test_page) - reinterpret_cast<std::uintptr_t>(test)) < 10);
    CHECK((ida_sig.find_in<true>(*test_page).front() - reinterpret_cast<std::uintptr_t>(test)) < 10);

    CHECK((*ida_sig.find_in<false>(*this_module) - reinterpret_cast<std::uintptr_t>(test)) < 10);
    CHECK((ida_sig.find_in<true>(*this_module).front() - reinterpret_cast<std::uintptr_t>(test)) < 10);

    auto code_sig = lime::sig("\x00\xC7\x00\x39\x05\x00\x00\x48\x2D\x95\x03\x00\x00", "?x?xxxxxxxxxx");
    CHECK((*code_sig.find<false>() - reinterpret_cast<std::uintptr_t>(test)) < 10);
    CHECK((code_sig.find<true>().front() - reinterpret_cast<std::uintptr_t>(test)) < 10);

    CHECK((*code_sig.find_in<false>(*test_page) - reinterpret_cast<std::uintptr_t>(test)) < 10);
    CHECK((code_sig.find_in<true>(*test_page).front() - reinterpret_cast<std::uintptr_t>(test)) < 10);

    CHECK((*code_sig.find_in<false>(*this_module) - reinterpret_cast<std::uintptr_t>(test)) < 10);
    CHECK((code_sig.find_in<true>(*this_module).front() - reinterpret_cast<std::uintptr_t>(test)) < 10);

    //
    CHECK(ida_sig.find<true>() == code_sig.find<true>());
}