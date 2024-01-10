#include <boost/ut.hpp>
#include <lime/instruction.hpp>

using namespace boost::ut;
using namespace boost::ut::literals;

int func2(int a, int b)
{
    return a + b;
}

int func1(int a)
{
    if (a > 10)
    {
        return 20;
    }

    return func2(a, 30);
}

suite<"Instruction"> instruction_suite = []
{
    auto instruction = lime::instruction::at(reinterpret_cast<std::uintptr_t>(func1));

    expect(eq(instruction.has_value(), true));
    expect(eq(instruction->addr(), reinterpret_cast<std::uintptr_t>(func1)));

    static constexpr auto MNEMONIC_JLE = 306;
    auto jump                          = instruction->next(MNEMONIC_JLE);

    expect(eq(jump.has_value(), true));
    expect(eq(jump->relative(), true));
    expect(eq(jump->branching(), true));

    expect(gt(jump->size(), 0));
    expect(eq(jump->mnemonic(), MNEMONIC_JLE));
    expect(gt(jump->addr(), instruction->addr()));

    static constexpr auto MNEMONIC_CALL = 67;
    auto call                           = instruction->next(MNEMONIC_CALL);

    expect(eq(call.has_value(), true));
    expect(eq(call->relative(), true));

    expect(gt(call->size(), 0));
    expect(eq(call->mnemonic(), MNEMONIC_CALL));
    expect(gt(call->addr(), instruction->addr()));

    /*
     * We depend on compiler generated assembly here which isn't always what we'd expect it to be.
     * This test would probably be better suited on a small shell-code (TODO).
     *
     * As clang and gcc both compile `func1` and `func2` into roughly what we'd expect while MSVC does not the following
     * asserts are only enabled for Windows under clang-cl
     */

#if !defined(_MSC_VER) || defined(__clang__)
    auto follow = call->follow();

    expect(eq(follow.has_value(), true));
    expect(eq(follow->addr(), reinterpret_cast<std::uintptr_t>(func2)));

    auto next = follow->next();

    expect(eq(next.has_value(), true));
    expect(gt(next->addr(), follow->addr()));

    auto prev = next->prev();

    expect(eq(prev.has_value(), true));
    expect(eq(prev->addr(), follow->addr()));
    expect(eq(prev->mnemonic(), follow->mnemonic()));
#endif
};
