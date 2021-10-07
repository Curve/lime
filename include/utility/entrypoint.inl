#pragma once

#if defined(__linux__)
__attribute__((constructor)) inline void on_entry()
{
    entry();
}

__attribute__((destructor)) inline void on_exit()
{
    exit();
}
#elif defined(_WIN32)
#include <Windows.h>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        entry(hinstDLL, fdwReason, lpReserved);
        break;

    case DLL_PROCESS_DETACH:
        exit(hinstDLL, fdwReason, lpReserved);
        break;
    }

    return TRUE;
}
#endif