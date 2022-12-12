#include <lime/constants/mnemonics.hpp>
#include <lime/utility/signature.hpp>
#include <catch2/catch.hpp>
#include <lime/module.hpp>
#include <lime/page.hpp>

void another_function()
{
    printf("Function was called!");
}

void call_another_function()
{
    another_function();
}

const int some_test_int = 10;
const std::uint8_t code[] = {0x50, 0x48, 0xC7, 0xC0, 0xE8, 0x03, 0x00, 0x00, 0x58};
const std::uint8_t code2[] = {0x48, 0x8B, 0x3D, 0x6A, 0x50, 0xAE, 0x00};

TEST_CASE("Address utility is tested", "[address]")
{
    auto address = lime::address(reinterpret_cast<std::uintptr_t>(call_another_function));

    if (address.get_mnemonic() == lime::mnemonic::jmp)
        address = *address.follow();

    auto function_call = address.read_until(lime::mnemonic::call);
    REQUIRE(function_call);

    auto function_address = function_call->follow();
    REQUIRE(function_address);

    CHECK(function_address->get() == reinterpret_cast<std::uintptr_t>(another_function));

    auto test_address = lime::address(reinterpret_cast<std::uintptr_t>(&some_test_int));
    REQUIRE(test_address.get_as<int>() == 10);

    auto safe_value = test_address.get_as_safe<int>();
    REQUIRE(safe_value);
    CHECK(*safe_value == 10);

    auto test_code = lime::address(reinterpret_cast<std::uintptr_t>(code));
    REQUIRE(test_code.get_mnemonic().value_or(0) == lime::mnemonic::push);
    CHECK(test_code.next()->get_mnemonic() == lime::mnemonic::mov);

    auto immediates = test_code.next()->get_immediates();
    REQUIRE(!immediates.empty());
    REQUIRE(immediates.front() == 1000);

    auto test_code2 = lime::address(reinterpret_cast<std::uintptr_t>(code2));
    auto immediates2 = test_code2.get_immediates();
    REQUIRE(!immediates2.empty());
}