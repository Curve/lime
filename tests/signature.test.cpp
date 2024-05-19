#include <lime/utils/signature.hpp>
#include <boost/ut.hpp>

using namespace boost::ut;
using namespace boost::ut::literals;

struct impl
{
    std::string mask;
    std::string pattern;
};

suite<"Signature"> signature_suite = []
{
    const auto *ida_signature = "0F 2F 05 D8 20 ?? 03 72 41";
    const auto *code_pattern  = "\x0F\x2F\x05\xD8\x20\x00\x03\x72\x41";
    const auto *code_mask     = "xxxxx?xxx";

    auto ida_sig  = lime::signature::from(ida_signature);
    auto code_sig = lime::signature::from(code_pattern, code_mask);

    auto &ida_impl  = *reinterpret_cast<std::unique_ptr<impl> *>(&ida_sig);
    auto &code_impl = *reinterpret_cast<std::unique_ptr<impl> *>(&code_sig);

    expect(eq(ida_impl->mask, code_impl->mask));
    expect(eq(ida_impl->pattern, code_impl->pattern));
};
