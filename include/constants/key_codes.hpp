#pragma once
#ifdef __linux__
#define XK_LATIN1 1
#define XK_MISCELLANY 1
#include <X11/keysymdef.h>
#endif

namespace lime
{
    namespace key
    {
#ifdef __linux__
        enum : int
        {
            a = XK_A,
            b = XK_B,
            c = XK_C,
            d = XK_D,
            e = XK_E,
            f = XK_F,
            g = XK_G,
            h = XK_H,
            i = XK_I,
            j = XK_J,
            k = XK_K,
            l = XK_L,
            m = XK_M,
            n = XK_N,
            o = XK_O,
            p = XK_P,
            q = XK_Q,
            r = XK_R,
            s = XK_S,
            t = XK_T,
            u = XK_U,
            v = XK_V,
            w = XK_W,
            x = XK_X,
            y = XK_Y,
            z = XK_Z,
            alt = XK_Alt_L,
            shift = XK_Shift_L,
            ctrl = XK_Control_L,
            space = XK_space,
            rtn = XK_Return,
            F1 = XK_F1,
            F2 = XK_F2,
            F3 = XK_F3,
            F4 = XK_F4,
            F5 = XK_F5,
            F6 = XK_F6,
            F7 = XK_F7,
            F8 = XK_F8,
            F9 = XK_F9,
            F10 = XK_F10,
            F11 = XK_F11,
            F12 = XK_F12,
            insert = XK_Insert,
            del = XK_Delete,
            up = XK_Up,
            down = XK_Down,
            left = XK_Left,
            right = XK_Right,
            tab = XK_Tab,
            escape = XK_Escape,
            numpad_0 = XK_KP_0,
            numpad_1 = XK_KP_1,
            numpad_2 = XK_KP_2,
            numpad_3 = XK_KP_3,
            numpad_4 = XK_KP_4,
            numpad_5 = XK_KP_5,
            numpad_6 = XK_KP_6,
            numpad_7 = XK_KP_7,
            numpad_8 = XK_KP_8,
            numpad_9 = XK_KP_9,
            plus = XK_plus,
            minus = XK_minus,
        };
#endif
    } // namespace key
} // namespace lime