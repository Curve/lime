#pragma once
#include "entrypoint.hpp"

#if defined(__linux__)
namespace internal
{
    static auto _init = []() {
        entry();
        return true;
    }();

    __attribute__((destructor)) inline void _quit()
    {
        quit();
    }
} // namespace internal

#elif defined(_WIN32)
#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        entry(hinstDLL, fdwReason, lpReserved);
        break;

    case DLL_PROCESS_DETACH:
        quit(hinstDLL, fdwReason, lpReserved);
        break;
    }

    return TRUE;
}
#endif