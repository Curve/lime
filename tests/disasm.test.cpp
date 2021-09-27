#include "../src/disasm/disasm.hpp"
#include <catch2/catch.hpp>

TEST_CASE("Disassembly code is tested", "[disasm]")
{
    std::vector<std::uint8_t> near_code = {0x77, 0x10, 0x77, 0x18, 0x4C, 0x89, 0xC7, 0x4C, 0x89, 0xE2, 0xBE, 0x30, 0x00, 0x00, 0x00, 0x4C,
                                           0x89, 0x44, 0x24, 0x20, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x8D, 0x05, 0x37, 0x13, 0x00, 0x00};

    CHECK_FALSE(lime::disasm::is_far_relocateable(near_code));
    CHECK(lime::disasm::get_estimated_size(near_code, false) >= lime::disasm::build_code(near_code, 0, 0, 100)->size());
    CHECK_FALSE(lime::disasm::build_code(near_code, 0, 0, INT64_MAX));

    CHECK(lime::disasm::get_required_prologue_length(reinterpret_cast<std::uintptr_t>(near_code.data()), 6) == 7);

    //

    std::vector<std::uint8_t> far_code = {0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x48, 0x83, 0xF8, 0x00, 0x74, 0x05};

    CHECK(lime::disasm::is_far_relocateable(far_code));
    CHECK(lime::disasm::get_estimated_size(far_code, false) >= lime::disasm::build_code(far_code, 0, 0, 100)->size());
    CHECK(lime::disasm::get_estimated_size(far_code, true) >= lime::disasm::build_code(far_code, 0, 0, INT64_MAX)->size());

    CHECK(lime::disasm::get_required_prologue_length(reinterpret_cast<std::uintptr_t>(far_code.data()), 6) == 7);
}