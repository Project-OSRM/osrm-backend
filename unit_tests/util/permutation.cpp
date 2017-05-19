#include "../common/range_tools.hpp"

#include "util/permutation.hpp"

#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <numeric>
#include <random>

BOOST_AUTO_TEST_SUITE(permutation_test)

using namespace osrm;
using namespace osrm::util;

BOOST_AUTO_TEST_CASE(basic_permuation)
{
    // cycles (0 3 2 1 4 8) (5) (6 9 7)
    //                                            0  1  2  3  4  5  6  7   8   9
    std::vector<std::uint32_t> values{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    const std::vector<std::uint32_t> permutation{3, 4, 1, 2, 8, 5, 9, 6, 0, 7};
    std::vector<std::uint32_t> reference{9, 3, 4, 1, 2, 6, 8, 10, 5, 7};

    inplacePermutation(values.begin(), values.end(), permutation);

    CHECK_EQUAL_COLLECTIONS(values, reference);
}

BOOST_AUTO_TEST_SUITE_END()
