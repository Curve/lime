#pragma once

namespace lime
{
    class keyboard
    {
        struct impl;

      public:
        static void press(const int &);
        static void release(const int &);

        static bool is_down(const int &);
        static bool was_pressed(const int &);
    };
} // namespace lime