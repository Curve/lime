#include <boost/ut.hpp>

#include <filesystem>
#include <lime/lib.hpp>

#if defined(WIN32) || defined(_WIN32)
#include <windows.h>
#endif

using namespace boost::ut;
using namespace boost::ut::literals;

namespace fs = std::filesystem;

fs::path find_library()
{
#if defined(WIN32) || defined(_WIN32)
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, sizeof(path));

    return fs::path{path}.parent_path() / "lime-shared-lib.dll";
#else
    const auto self = fs::canonical("/proc/self/exe").parent_path();
    return self / "lime-shared-lib.so";
#endif
}

suite<"Module"> module_suite = []
{
    using namespace lime::literals;

    expect(eq(lime::lib::find("lime-shared-lib").has_value(), false));

    const auto path = find_library().string();

    auto loaded = lime::lib::load(path);
    expect(loaded.has_value());

    auto test = lime::lib::find("lime-shared-lib"_re);
    expect(test.has_value());

#if defined(WIN32) || defined(_WIN32)
    auto case_test = lime::lib::find("LIME-SHARED-LIB"_re);
    expect(case_test.has_value());
#endif

    expect(test->name().contains("lime-shared"));
    expect(test->symbols().size() >= 1);
    expect(test->size() > 0);

    expect(test->symbol("demo"_re).has_value());
    expect(eq(test->symbol("demo"_re).value(), (*test)["demo"_re].value()));

    expect(test->symbol("lime_demo_export").has_value());
    expect(test->symbol([](std::string_view name) { return name.contains("lime_demo"); }).has_value());

    expect(test->start() > 0);
    expect(test->end() > 0);

    using demo_export_t = int (*)(int);
    auto demo_export    = reinterpret_cast<demo_export_t>(*test->symbol("lime_demo_export"));

    expect(eq(demo_export(10), 20));
    expect(eq((*test)["lime_demo"_re, lime::id<int(int)>].value()(10), 20));
};
