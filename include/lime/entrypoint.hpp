#pragma once

namespace lime
{
    extern void load();
    extern void unload();
} // namespace lime

#if LIME_STATIC_ENTRYPOINT
#include <memory>

// NOLINTNEXTLINE(cert-err58-cpp, *-namespace)
[[maybe_unused]] static auto constructor = []()
{
    lime::load();
    return 1;
}();

// NOLINTNEXTLINE(cert-err58-cpp, *-namespace)
[[maybe_unused]] static auto destructor = []()
{
    return std::shared_ptr<char>(new char,
                                 [](auto *data)
                                 {
                                     lime::unload();
                                     delete data;
                                 });
}();
#elif defined(WIN32) || defined(_WIN32)
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
#else
void __attribute__((constructor)) constructor()
{
    lime::load();
}

void __attribute__((destructor)) destructor()
{
    lime::unload();
}
#endif
