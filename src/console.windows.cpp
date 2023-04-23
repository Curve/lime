#include "utility/console.hpp"

#include <windows.h>
#include <memory>
#include <fstream>
#include <iostream>
#include <functional>

namespace lime
{
    template <typename T> using deleted_unique_ptr = std::unique_ptr<T, std::function<void(T *)>>;

    struct console::impl
    {
        static HANDLE console_handle;
        static std::streambuf *original_stream;
        static deleted_unique_ptr<std::ofstream> custom_stream;
    };

    std::streambuf *console::impl::original_stream;
    deleted_unique_ptr<std::ofstream> console::impl::custom_stream;

    void console::restore()
    {
        if (impl::original_stream)
        {
            std::cout.rdbuf(impl::original_stream);
            impl::custom_stream.reset();
        }
    }

    void console::redirect_to_file(const std::filesystem::path &path)
    {
        if (!impl::original_stream)
        {
            impl::original_stream = std::cout.rdbuf();
        }

        impl::custom_stream = std::make_unique<std::ofstream>(path);
        std::cout.rdbuf(impl::custom_stream->rdbuf());
    }

    void console::alloc_console(const std::string &name)
    {
        if (!impl::original_stream)
        {
            impl::original_stream = std::cout.rdbuf();
        }

        if (AllocConsole())
        {
            if (!name.empty())
                SetConsoleTitle(name.c_str());

            impl::custom_stream = deleted_unique_ptr<std::ofstream>(new std::ofstream("CONOUT$"), [name](std::ostream *data) {
                delete data;
                FreeConsole();
            });

            std::cout.rdbuf(impl::custom_stream->rdbuf());
        }
    }
} // namespace lime