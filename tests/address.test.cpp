#include <random>
#include <boost/ut.hpp>
#include <lime/address.hpp>
#include <ranges>

using namespace boost::ut;
using namespace boost::ut::literals;

suite<"Address"> address_suite = []
{
    auto seed = std::seed_seq{std::random_device{}()};
    std::mt19937 random{seed};

    static constexpr auto n      = 50;
    static constexpr auto offset = sizeof(int);

    std::vector<int> data;
    data.reserve(n);

    for (auto i = 0u; n > i; i++)
    {
        data.emplace_back(random());
    }

    auto address = lime::address::at(reinterpret_cast<std::uintptr_t>(data.data()));
    {
        expect(eq(address.has_value(), true));
        expect(eq(address->addr(), reinterpret_cast<std::uintptr_t>(data.data())));

        for (const auto &value : data)
        {
            auto &current = address.value();
            expect(eq(current.read<int>(), value));

            auto next = current + offset;
            expect(eq(next.has_value(), true));

            address.emplace(std::move(next.value()));
        }
    }

    auto back_address = lime::address::at(address->addr() - offset);
    {
        expect(eq(back_address.has_value(), true));

        for (const auto &value : data | std::ranges::views::reverse)
        {
            auto &current = back_address.value();

            expect(eq(current.read<int>(), value));
            current.write(1337);

            auto next = current - offset;
            expect(eq(next.has_value(), true));

            back_address.emplace(std::move(next.value()));
        }
    }

    for (const auto &value : data)
    {
        expect(eq(value, 1337));
    }
};
