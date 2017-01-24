#include "util/group_by.hpp"

#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include <iterator>
#include <vector>

BOOST_AUTO_TEST_SUITE(group_by_test)

using namespace osrm;
using namespace osrm::util;

namespace
{

struct Yes
{
    template <typename T> bool operator()(T &&) { return true; }
};

struct No
{
    template <typename T> bool operator()(T &&) { return false; }
};

struct Alternating
{
    template <typename T> bool operator()(T &&) { return state = !state; }
    bool state = true;
};

struct SubRangeCounter
{
    template <typename Range> void operator()(Range &&) { count += 1; }
    std::size_t count = 0;
};
}

BOOST_AUTO_TEST_CASE(grouped_empty_test)
{
    std::vector<int> v{};
    auto ranges = group_by(begin(v), end(v), Yes{}, SubRangeCounter{});
    BOOST_CHECK_EQUAL(ranges.count, 0);
}

BOOST_AUTO_TEST_CASE(grouped_all_match_range_test)
{
    std::vector<int> v{1, 2, 3};
    auto ranges = group_by(begin(v), end(v), Yes{}, SubRangeCounter{});
    BOOST_CHECK_EQUAL(ranges.count, 1);
}

BOOST_AUTO_TEST_CASE(grouped_no_match_range_test)
{
    std::vector<int> v{1, 2, 3};
    auto ranges = group_by(begin(v), end(v), No{}, SubRangeCounter{});
    BOOST_CHECK_EQUAL(ranges.count, 1);
}

BOOST_AUTO_TEST_CASE(grouped_alternating_matches_range_test)
{
    std::vector<int> v{1, 2, 3};
    auto ranges = group_by(begin(v), end(v), Alternating{}, SubRangeCounter{});
    BOOST_CHECK_EQUAL(ranges.count, v.size());
}

BOOST_AUTO_TEST_SUITE_END()
