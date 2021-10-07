#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <utility/console.hpp>

namespace lime
{
    template <typename T> using deleted_unique_ptr = std::unique_ptr<T, std::function<void(T *)>>;

    struct console::impl
    {
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
        // ? On linux we will create a file in /tmp/ and write our output there and then watch the output with `tail`.
        // ? We do this because opening an actual terminal isn't portable at all

        if (!impl::original_stream)
        {
            impl::original_stream = std::cout.rdbuf();
        }

        impl::custom_stream =
            deleted_unique_ptr<std::ofstream>(new std::ofstream(name.empty() ? "/tmp/lime" : "/tmp/lime_" + name), [name](std::ostream *data) {
                delete data;
                std::filesystem::remove(name.empty() ? "/tmp/lime" : "/tmp/lime_" + name);
            });

        std::cout.rdbuf(impl::custom_stream->rdbuf());
    }
} // namespace lime