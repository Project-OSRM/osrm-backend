#include "util/alias.hpp"
#include "util/integer_range.hpp"
#include "util/log.hpp"
#include "util/timing_util.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

using namespace osrm;

namespace tag
{
struct interger_alias
{
};
struct double_alias
{
};
} // namespace tag

int main(int, char **)
{
    util::LogPolicy::GetInstance().Unmute();

    auto num_rounds = 1000;
    auto num_entries = 1000000;

    std::vector<std::size_t> indices(num_entries);
    std::iota(indices.begin(), indices.end(), 0);
    std::mt19937 g(1337);
    std::shuffle(indices.begin(), indices.end(), g);

    using osrm_uint32 = Alias<std::uint32_t, tag::interger_alias>;
    std::vector<osrm_uint32> aliased_uint32(num_entries);
    std::vector<std::uint32_t> plain_uint32(num_entries);

    using osrm_double = Alias<double, tag::double_alias>;
    std::vector<osrm_double> aliased_double(num_entries);
    std::vector<double> plain_double(num_entries);

    std::iota(aliased_uint32.begin(), aliased_uint32.end(), osrm_uint32{0});
    std::iota(plain_uint32.begin(), plain_uint32.end(), 0);
    std::iota(aliased_double.begin(), aliased_double.end(), osrm_double{1.0});
    std::iota(plain_double.begin(), plain_double.end(), 1.0);

    TIMER_START(aliased_u32);
    for (auto round : util::irange(0, num_rounds))
    {
        (void)round;
        osrm_uint32 sum{0};
        osrm_uint32 mult{1};
        for (auto idx : indices)
        {
            sum += aliased_uint32[idx];
            mult *= aliased_uint32[idx];
        }
        if (sum != osrm_uint32{1783293664})
            return EXIT_FAILURE;
        if (mult != osrm_uint32{0})
            return EXIT_FAILURE;
    }
    TIMER_STOP(aliased_u32);
    std::cout << "aliased u32: " << TIMER_MSEC(aliased_u32) << std::endl;

    TIMER_START(plain_u32);
    for (auto round : util::irange(0, num_rounds))
    {
        (void)round;
        std::uint32_t sum{0};
        std::uint32_t mult{1};
        for (auto idx : indices)
        {
            sum += plain_uint32[idx];
            mult *= plain_uint32[idx];
        }
        if (sum != 1783293664)
            return EXIT_FAILURE;
        if (mult != 0)
            return EXIT_FAILURE;
    }
    TIMER_STOP(plain_u32);
    std::cout << "plain u32: " << TIMER_MSEC(plain_u32) << std::endl;

    TIMER_START(aliased_double);
    for (auto round : util::irange(0, num_rounds))
    {
        (void)round;
        osrm_double sum{0.0};
        osrm_double mult{1.0};
        for (auto idx : indices)
        {
            sum += aliased_double[idx];
            mult *= aliased_double[idx];
        }

        if (sum != osrm_double{500000500000})
            return EXIT_FAILURE;
        if (mult != osrm_double{std::numeric_limits<double>::infinity()})
            return EXIT_FAILURE;
    }
    TIMER_STOP(aliased_double);
    std::cout << "aliased double: " << TIMER_MSEC(aliased_double) << std::endl;

    TIMER_START(plain_double);
    for (auto round : util::irange(0, num_rounds))
    {
        (void)round;
        double sum{0.0};
        double mult{1.0};
        for (auto idx : indices)
        {
            sum += plain_double[idx];
            mult *= plain_double[idx];
        }

        if (sum != 500000500000)
            return EXIT_FAILURE;
        if (mult != std::numeric_limits<double>::infinity())
            return EXIT_FAILURE;
    }
    TIMER_STOP(plain_double);
    std::cout << "plain double: " << TIMER_MSEC(plain_double) << std::endl;
}
