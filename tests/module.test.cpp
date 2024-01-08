#include <filesystem>
#include <boost/ut.hpp>
#include <lime/module.hpp>

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

    const auto self = fs::path{path}.parent_path();
    return self / "lime-shared-lib";
#else
    const auto self = fs::canonical("/proc/self/exe").parent_path();
    return self / "lime-shared-lib";
#endif
}

suite<"Module"> module_suite = []
{
    expect(eq(lime::module::find("lime-shared-lib").has_value(), false));

    const auto path = find_library();

    std::cout << path.string() << std::endl;

    auto loaded = lime::module::load(path.string());
    expect(eq(loaded.has_value(), true));

    auto test = lime::module::find("lime-shared-lib");
    expect(eq(test.has_value(), true));

#if defined(WIN32) || defined(_WIN32)
    auto case_test = lime::module::find("LIME-SHARED-LIB");
    expect(eq(case_test.has_value(), true));
#endif

    expect(test->name().find("lime-shared") != std::string_view::npos);

    expect(test->symbols().size() >= 1);
    expect(test->size() > 0);

    expect(eq(test->find_symbol("lime_demo").has_value(), true));
    expect(test->symbol("lime_demo_export") > 0);

    expect(test->start() > 0);
    expect(test->end() > 0);

    using demo_export_t = int (*)(int);
    auto demo_export    = reinterpret_cast<demo_export_t>(test->symbol("lime_demo_export"));

    expect(eq(demo_export(10), 20));
};
