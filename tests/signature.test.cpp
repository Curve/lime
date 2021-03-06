#include <lime/utility/signature.hpp>
#include <catch2/catch.hpp>
#include <lime/module.hpp>
#include <lime/page.hpp>

static const std::uint8_t find_me[] = {0x90, 0xC7, 0x10, 0x39, 0x05, 0x00, 0x00, 0x00, 0x48, 0x2D, 0x95, 0x03, 0x00, 0x00};

TEST_CASE("Signature scanning is tested", "[signature]")
{
    auto test_page = lime::page::get_page_at(reinterpret_cast<std::uintptr_t>(&find_me));
    auto this_module = lime::module::get("");

    REQUIRE(test_page);
    REQUIRE(this_module);

    auto ida_sig = lime::sig("?? C7 ? 39 05 00 00 48 2D 95 03 00 00");
    CHECK(!ida_sig.find_in<true>(*test_page).empty());
    CHECK(!ida_sig.find_in<true>(*this_module).empty());

    auto code_sig = lime::sig("\x00\xC7\x00\x39\x05\x00\x00\x48\x2D\x95\x03\x00\x00", "?x?xxxxxxxxxx");
    CHECK(!code_sig.find_in<true>(*test_page).empty());
    CHECK(!code_sig.find_in<true>(*this_module).empty());

    CHECK(ida_sig.find_in<true>(*test_page) == code_sig.find_in<true>(*test_page));
}