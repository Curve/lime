#include "utility/keyboard.hpp"

#include <memory>
#include <functional>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

namespace lime
{
    template <typename T> using deleted_unique_ptr = std::unique_ptr<T, std::function<void(T *)>>;

    struct keyboard::impl
    {
        static deleted_unique_ptr<Display> display;
        static char last_key_state[32];
    };

    char keyboard::impl::last_key_state[32] = "";
    deleted_unique_ptr<Display> keyboard::impl::display = []() -> deleted_unique_ptr<Display> {
        auto *env = std::getenv("DISPLAY"); // NOLINT
        auto *display = XOpenDisplay(env);

        if (!display && !(display = XOpenDisplay(":0")))
            return nullptr;

        return {display, [](Display *data) { XCloseDisplay(data); }};
    }();

    bool keyboard::is_down(const int &key)
    {
        if (impl::display)
        {
            XQueryKeymap(impl::display.get(), impl::last_key_state);
            auto key_code = XKeysymToKeycode(impl::display.get(), key);

            return !!(impl::last_key_state[key_code >> 3] & (1 << (key_code & 7)));
        }

        return false;
    }

    bool keyboard::was_pressed(const int &key)
    {
        if (impl::display)
        {
            char key_state[32];
            XQueryKeymap(impl::display.get(), key_state);
            auto key_code = XKeysymToKeycode(impl::display.get(), key);

            auto pressed = !!(key_state[key_code >> 3] & (1 << (key_code & 7)));
            auto was_pressed = !!(impl::last_key_state[key_code >> 3] & (1 << (key_code & 7)));

            std::copy(std::begin(key_state), std::end(key_state), std::begin(impl::last_key_state));
            return pressed && !was_pressed;
        }

        return false;
    }

    void keyboard::press(const int &key)
    {
        if (impl::display)
        {
            XTestFakeKeyEvent(impl::display.get(), XKeysymToKeycode(impl::display.get(), key), True, 0);
        }
    }

    void keyboard::release(const int &key)
    {
        if (impl::display)
        {
            XTestFakeKeyEvent(impl::display.get(), XKeysymToKeycode(impl::display.get(), key), False, 0);
        }
    }
} // namespace lime