#include <boost/ut.hpp>

#include <cmath>
#include <lime/instruction.hpp>

using namespace boost::ut;
using namespace boost::ut::literals;

std::vector<unsigned char> code = {
#if INTPTR_MAX == INT64_MAX
    0x8b, 0x02,                               // mov eax, [rdx]
    0x4c, 0x8b, 0xc1,                         // mov [rcx], eax
    0x48, 0x8B, 0x05, 0x05, 0x00, 0x00, 0x00, // mov rax, [rip+0x5]
    0xff, 0x25, 0x10, 0x00, 0x00, 0x00,       // jmp [rip+0x10]
    0xff, 0xe0,                               // jmp rax
#else
    0x8B, 0x02,                         // mov eax, [edx]
    0x89, 0x01,                         // mov [ecx], eax
    0xA1, 0x05, 0x00, 0x00, 0x00,       // mov eax, [rip+0x5]
    0xFF, 0x25, 0x10, 0x00, 0x00, 0x00, // jmp [rip+0x10]
    0xFF, 0xE0,                         // jmp eax
#endif
};

suite<"Instruction"> instruction_suite = []
{
    static constexpr auto MNEMONIC_MOV = 436;
    static constexpr auto MNEMONIC_JMP = 311;

    auto *data = code.data();

    auto instruction = lime::instruction::at(reinterpret_cast<std::uintptr_t>(data));

    expect(eq(instruction.has_value(), true));

    expect(eq(instruction->size(), 2));
    expect(eq(instruction->addr(), reinterpret_cast<std::uintptr_t>(data)));

    expect(eq(instruction->relative(), false));
    expect(eq(instruction->branching(), false));
    expect(eq(instruction->mnemonic(), MNEMONIC_MOV));

    auto mov = instruction->next();

    expect(eq(mov.has_value(), true));

    expect(eq(mov->relative(), false));
    expect(eq(mov->branching(), false));
    expect(eq(mov->mnemonic(), MNEMONIC_MOV));

    auto prev = mov->prev();

    expect(eq(prev.has_value(), true));
    expect(eq(prev->mnemonic(), MNEMONIC_MOV));

    expect(eq(prev->size(), 2));
    expect(eq(prev->addr(), reinterpret_cast<std::uintptr_t>(data)));

    auto rel_move = mov->next();

    expect(eq(rel_move.has_value(), true));

    expect(eq(rel_move->relative(), true));
    expect(eq(rel_move->branching(), false));
    expect(eq(rel_move->mnemonic(), MNEMONIC_MOV));

    auto jmp       = rel_move->next();
    auto jmp_until = instruction->next(MNEMONIC_JMP);

    expect(eq(jmp.has_value(), true));
    expect(eq(jmp_until.has_value(), true));

    expect(eq(jmp->addr(), jmp_until->addr()));

    expect(eq(jmp->relative(), true));
    expect(eq(jmp->branching(), true));
    expect(eq(jmp->mnemonic(), MNEMONIC_JMP));

    auto abs_jmp = jmp->next();

    expect(eq(abs_jmp.has_value(), true));

    expect(eq(abs_jmp->relative(), false));
    expect(eq(abs_jmp->branching(), true));
    expect(eq(abs_jmp->mnemonic(), MNEMONIC_JMP));

#if INTPTR_MAX == INT64_MAX
    expect(eq(next->size(), 3));
    expect(eq(next->addr(), reinterpret_cast<std::uintptr_t>(data) + 0x2));

    expect(eq(rel_move->size(), 7));
    expect(eq(rel_move->addr(), reinterpret_cast<std::uintptr_t>(data) + 15));

    expect(eq(jmp->size(), 6));
    expect(eq(jmp->addr(), rel_move->addr() + rel_move->size()));

    expect(eq(abs_jmp->size(), 2));
    expect(eq(abs_jmp->addr(), jmp->addr() + jmp->size()));
#endif
};
