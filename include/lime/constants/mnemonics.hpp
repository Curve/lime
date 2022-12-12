#pragma once

namespace lime
{
    struct mnemonic
    {
        static const int ret;
        static const int jmp;
        static const int lea;
        static const int cmp;
        static const int mov;

        static const int call;
        static const int push;
    };
} // namespace lime