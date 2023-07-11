#include "entrypoint.hpp"

#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        lime::load();
    }

    if (reason == DLL_PROCESS_DETACH)
    {
        if (reserved)
        {
            return TRUE;
        }

        lime::unload();
    }

    return TRUE;
}