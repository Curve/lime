#include "entrypoint.hpp"

#include <memory>

// NOLINTNEXTLINE(cert-err58-cpp, *-namespace)
[[maybe_unused]] static auto constructor = []() {
    lime::load();
    return 1;
}();

// NOLINTNEXTLINE(cert-err58-cpp, *-namespace)
[[maybe_unused]] static auto destructor = []() {
    return std::shared_ptr<char>(new char, [](auto *data) {
        lime::unload();
        delete data;
    });
}();
