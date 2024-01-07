#include <filesystem>
#include <lime/module.hpp>
#include <boost/ut.hpp>

using namespace boost::ut;
using namespace boost::ut::literals;

namespace fs = std::filesystem;

std::optional<fs::path> find_library()
{
    for (const auto &entry : fs::recursive_directory_iterator{fs::current_path()})
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        const auto &path = entry.path();
        const auto name  = path.filename().string();

        if (!name.starts_with("lime-shared-lib"))
        {
            continue;
        }

        return path;
    }

    return std::nullopt;
}

suite<"Module"> module_suite = []
{
    expect(eq(lime::module::find("lime-shared-lib").has_value(), false));

    auto path = find_library();
    expect(eq(path.has_value(), true));

    auto loaded = lime::module::load(path->string());
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
