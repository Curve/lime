#include <Windows.h>
#include <utility/keyboard.hpp>

namespace lime
{
    bool keyboard::is_down(const int &key)
    {
        return GetAsyncKeyState(key);
    }

    bool keyboard::was_pressed(const int &key)
    {
        return GetAsyncKeyState(key) & 1;
    }

    void keyboard::press(const int &key)
    {
        INPUT inputs[1] = {};
        ZeroMemory(inputs, sizeof(inputs));

        inputs[0].ki.wVk = key;
        inputs[0].type = INPUT_KEYBOARD;

        SendInput(1, inputs, sizeof(INPUT));
    }

    void keyboard::release(const int &key)
    {
        INPUT inputs[1] = {};
        ZeroMemory(inputs, sizeof(inputs));

        inputs[0].ki.wVk = key;
        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;

        SendInput(1, inputs, sizeof(INPUT));
    }
} // namespace lime