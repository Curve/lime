#include <utility/entrypoint.hpp>

__attribute__((constructor)) static void on_entry()
{
    lime::entry();
}

__attribute__((destructor)) static void on_exit()
{
    lime::exit();
}