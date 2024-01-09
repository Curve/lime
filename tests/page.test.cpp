#include <boost/ut.hpp>
#include <lime/page.hpp>

using namespace boost::ut;
using namespace boost::ut::literals;

int example(int arg)
{
    return arg + 20;
}

suite<"Page"> page_suite = []
{
    using prot = lime::protection;

    expect(gt(lime::page::pages().size(), 0));

    auto addr = reinterpret_cast<std::uintptr_t>(example);
    {
        auto page = lime::page::at(addr);

        expect(eq(page.has_value(), true));

        expect(page->prot() & prot::read);
        expect(page->prot() & prot::execute);

        expect(ge(addr, page->start()));
        expect(le(addr, page->end()));
    }

    auto allocated = lime::page::allocate(100, prot::read | prot::write | prot::execute);
    {
        expect(neq(allocated, nullptr));

        auto page = lime::page::at(allocated->start());

        expect(eq(page.has_value(), true));

        expect(page->prot() & prot::read);
        expect(page->prot() & prot::write);
        expect(page->prot() & prot::execute);
    }

    auto near = lime::page::allocate(addr, 10, prot::read);
    {
        expect(neq(near, nullptr));

        auto page = lime::page::at(near->start());

        expect(eq(page.has_value(), true));

        expect(page->prot() & prot::read);
        expect(not(page->prot() & prot::write));
        expect(not(page->prot() & prot::execute));

        const auto diff = std::abs(static_cast<std::intptr_t>(near->start()) - static_cast<std::intptr_t>(addr));

        expect(gt(diff, 0));
        expect(lt(diff, std::numeric_limits<std::uint32_t>::max()));
    }
};
