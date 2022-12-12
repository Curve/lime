#include "constants/mnemonics.hpp"
#include <Zydis/Zydis.h>

namespace lime
{
    const int mnemonic::ret = ZYDIS_MNEMONIC_RET;
    const int mnemonic::jmp = ZYDIS_MNEMONIC_JMP;
    const int mnemonic::lea = ZYDIS_MNEMONIC_LEA;
    const int mnemonic::cmp = ZYDIS_MNEMONIC_CMP;
    const int mnemonic::mov = ZYDIS_MNEMONIC_MOV;

    const int mnemonic::call = ZYDIS_MNEMONIC_CALL;
    const int mnemonic::push = ZYDIS_MNEMONIC_PUSH;
} // namespace lime