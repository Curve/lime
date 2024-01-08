#include <boost/ut.hpp>
#include <lime/module.hpp>

using namespace boost::ut;
using namespace boost::ut::literals;

#ifdef __linux__
#include <filesystem>

namespace fs = std::filesystem;

fs::path find_library()
{
    const auto self = fs::canonical("/proc/self/exe").parent_path();
    return self / "lime-shared-lib";
}
#endif

suite<"Module"> module_suite = []
{
    expect(eq(lime::module::find("lime-shared-lib").has_value(), false));

#if defined(WIN32) || defined(_WIN32)
    const auto *path = "lime-shared-lib";
#else
    const auto path = find_library().string();
#endif

    auto loaded = lime::module::load(path);
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
