#pragma once
#ifdef __linux__
#include <sys/mman.h>
#endif

namespace lime
{
#ifdef __linux__
    enum prot
    {
        prot_read = PROT_READ,
        prot_write = PROT_WRITE,
        prot_execute = PROT_EXEC
    };
#endif
} // namespace lime