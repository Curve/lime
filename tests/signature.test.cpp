#include <lime/signature.hpp>
#include <boost/ut.hpp>

using namespace boost::ut;
using namespace boost::ut::literals;

volatile std::uint8_t data[] = {
    0x5b, 0x98, 0x05, 0x9a, 0xae, 0xcd, 0x1e, 0x2b, 0x1f, 0xda, 0x7d, 0x85, 0xc0,
    0x12, 0xb0, 0xcf, 0x4b, 0x89, 0xfc, 0x65, 0xc1, 0xf7, 0x92, 0x9b, 0x74,
};

suite<"Signature"> signature_suite = []
{
    using enum lime::signature::results;

    const auto address = reinterpret_cast<std::uintptr_t>(std::addressof(data));
    const auto page    = lime::page::at(address);

    expect(page.has_value());

    const auto ida_sig = lime::signature{"ae cd ?? 2b"};
    const auto sig     = lime::signature{"\xae\xcd\x00\x2b", "xx?x"};

    const auto ida_result     = ida_sig.find(*page);
    const auto pattern_result = sig.find(*page);

    expect(ida_result.has_value());
    expect(pattern_result.has_value());

    expect(eq(*ida_result, *pattern_result));
    expect(eq(*ida_result, address + 4));

    expect(std::ranges::contains(ida_sig.find<all>(*page), *ida_result));
    expect(std::ranges::contains(sig.find<all>(*page), *ida_result));
};
