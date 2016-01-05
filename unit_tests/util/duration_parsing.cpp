#include "extractor/extraction_helper_functions.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>

BOOST_AUTO_TEST_SUITE(durations_are_valid)

using namespace osrm;
using namespace osrm::util;

BOOST_AUTO_TEST_CASE(all_necessary_test)
{
    BOOST_CHECK_EQUAL(extractor::durationIsValid("00:01"), true);
    BOOST_CHECK_EQUAL(extractor::durationIsValid("00:01:01"), true);
    BOOST_CHECK_EQUAL(extractor::durationIsValid("PT15M"), true);
}

BOOST_AUTO_TEST_CASE(common_durations_get_translated)
{
    BOOST_CHECK_EQUAL(extractor::parseDuration("00:01"), 60);
    BOOST_CHECK_EQUAL(extractor::parseDuration("00:01:01"), 61);
    BOOST_CHECK_EQUAL(extractor::parseDuration("01:01"), 3660);

    // check all combinations of iso duration tokens
    BOOST_CHECK_EQUAL(extractor::parseDuration("PT1M1S"), 61);
    BOOST_CHECK_EQUAL(extractor::parseDuration("PT1H1S"), 3601);
    BOOST_CHECK_EQUAL(extractor::parseDuration("PT15M"), 900);
    BOOST_CHECK_EQUAL(extractor::parseDuration("PT15S"), 15);
    BOOST_CHECK_EQUAL(extractor::parseDuration("PT15H"), 54000);
    BOOST_CHECK_EQUAL(extractor::parseDuration("PT1H15M"), 4500);
    BOOST_CHECK_EQUAL(extractor::parseDuration("PT1H15M1S"), 4501);
}

BOOST_AUTO_TEST_CASE(iso_8601_durations_case_insensitive)
{
    BOOST_CHECK_EQUAL(extractor::parseDuration("PT15m"), 900);
    BOOST_CHECK_EQUAL(extractor::parseDuration("PT1h15m"), 4500);
}

BOOST_AUTO_TEST_SUITE_END()
