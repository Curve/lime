#include "entrypoint.hpp"

void __attribute__((constructor)) constructor()
{
    lime::load();
}

void __attribute__((destructor)) destructor()
{
    lime::unload();
}