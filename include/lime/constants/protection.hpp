#pragma once
#if defined(__linux__)
#include <sys/mman.h>
#elif defined(_WIN32)
#include <Windows.h>
#endif

namespace lime
{
    namespace prot
    {
#if defined(__linux__)
        enum
        {
            none = PROT_NONE,
            read_only = PROT_READ,
            read_write = PROT_WRITE | PROT_READ,

            execute = PROT_EXEC,
            read_execute = PROT_EXEC | PROT_READ,
            read_write_execute = PROT_EXEC | PROT_READ | PROT_WRITE,
        };
#elif defined(_WIN32)
        enum
        {
            none = PAGE_NOACCESS,
            read_only = PAGE_READONLY,
            read_write = PAGE_READWRITE,

            execute = PAGE_EXECUTE,
            read_execute = PAGE_EXECUTE_READ,
            read_write_execute = PAGE_EXECUTE_READWRITE,
        };
#endif
    } // namespace prot
} // namespace lime