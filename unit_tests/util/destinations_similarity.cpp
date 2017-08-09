#include "util/guidance/destinations_similarity.hpp"

#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include <iterator>
#include <vector>

BOOST_AUTO_TEST_SUITE(destination_similarity_test)

using namespace osrm;
using namespace osrm::util;
using namespace osrm::util::guidance;

BOOST_AUTO_TEST_CASE(similarity_test)
{
    struct Data
    {
        std::string lhs, rhs;
        double result;
    } const data[] = {{"", "", 1.},
                      {"A", "A", 1.},
                      {"A", "B", 0.},
                      {"A", "A, B", 0.5},
                      {"US 101 South,  US 101 North, I 380 West: San Jose, San Francisco",
                       "US 101, US 101 North, I 380 West",
                       1. / 3}};

    for (std::size_t i = 0; i < sizeof(data) / sizeof(data[0]); ++i)
    {
        const auto &lhs = getDestinationTokens(data[i].lhs);
        const auto &rhs = getDestinationTokens(data[i].rhs);
        BOOST_CHECK_EQUAL(getSetsSimilarity(lhs, rhs), data[i].result);
    }
}

BOOST_AUTO_TEST_SUITE_END()
