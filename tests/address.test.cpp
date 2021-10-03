#include <catch2/catch.hpp>
#include <constants/mnemonics.hpp>
#include <module.hpp>
#include <page.hpp>
#include <utility/signature.hpp>

void another_function()
{
    printf("Function was called!");
}

void call_another_function()
{
    another_function();
}

int some_test_int = 10;

TEST_CASE("Address utility is tested", "[address]")
{
    auto address = lime::address(reinterpret_cast<std::uintptr_t>(call_another_function));

    if (address.get_mnemonic() == lime::mnemonic::JMP)
        address = *address.follow();

    auto function_call = address.read_until(lime::mnemonic::CALL);
    REQUIRE(function_call);

    auto function_address = function_call->follow();
    REQUIRE(function_address);

    CHECK(function_address->get() == reinterpret_cast<std::uintptr_t>(another_function));

    auto test_address = lime::address(reinterpret_cast<std::uintptr_t>(&some_test_int));
    REQUIRE(test_address.get_as<int>() == 10);

    auto safe_value = test_address.get_as_safe<int>();
    REQUIRE(safe_value);
    CHECK(*safe_value == 10);
}