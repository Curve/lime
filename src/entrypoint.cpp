#include "entrypoint.hpp"

template <typename T>
struct defer
{
    T callback;

  public:
    ~defer()
    {
        callback();
    }
};

[[maybe_unused]] auto constructor = []
{
    lime::load();
    return nullptr;
}();

[[maybe_unused]] auto destructor = []
{
    return defer{[] { lime::unload(); }};
}();
