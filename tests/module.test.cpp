#include <filesystem>
#include <lime/module.hpp>
#include <boost/ut.hpp>

using namespace boost::ut;
using namespace boost::ut::literals;

suite<"Module"> module_suite = []
{
    expect(eq(lime::module::find("lime-shared-lib").has_value(), false));

    const auto path = std::filesystem::current_path() / "lime-shared-lib";
    auto loaded     = lime::module::load(path.string());
    expect(eq(loaded.has_value(), true));

    auto test = lime::module::find("lime-shared-lib");
    expect(eq(test.has_value(), true));

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
