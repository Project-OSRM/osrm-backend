#include "extractor/extraction_helper_functions.hpp"

#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(durations_are_valid)

using namespace osrm;

BOOST_AUTO_TEST_CASE(all_necessary_test)
{
    BOOST_CHECK_EQUAL(extractor::durationIsValid("0"), true);
    BOOST_CHECK_EQUAL(extractor::durationIsValid("00:01"), true);
    BOOST_CHECK_EQUAL(extractor::durationIsValid("00:01:01"), true);
    BOOST_CHECK_EQUAL(extractor::durationIsValid("61"), true);
    BOOST_CHECK_EQUAL(extractor::durationIsValid("24:01"), true);
    BOOST_CHECK_EQUAL(extractor::durationIsValid("00:01:60"), true);
    BOOST_CHECK_EQUAL(extractor::durationIsValid("PT15M"), true);

    BOOST_CHECK_EQUAL(extractor::durationIsValid(""), false);
    BOOST_CHECK_EQUAL(extractor::durationIsValid("PT15"), false);
    BOOST_CHECK_EQUAL(extractor::durationIsValid("PT15A"), false);
    BOOST_CHECK_EQUAL(extractor::durationIsValid("PT1H25:01"), false);
    BOOST_CHECK_EQUAL(extractor::durationIsValid("PT12501"), false);
    BOOST_CHECK_EQUAL(extractor::durationIsValid("PT0125:01"), false);
    BOOST_CHECK_EQUAL(extractor::durationIsValid("PT016001"), false);
    BOOST_CHECK_EQUAL(extractor::durationIsValid("PT240000"), false);
    BOOST_CHECK_EQUAL(extractor::durationIsValid("PT24:00:00"), false);
}

BOOST_AUTO_TEST_CASE(common_durations_get_translated)
{
    BOOST_CHECK_EQUAL(extractor::parseDuration("00"), 0);
    BOOST_CHECK_EQUAL(extractor::parseDuration("10"), 600);
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
    BOOST_CHECK_EQUAL(extractor::parseDuration("PT2H25M6S"), 8706);
    BOOST_CHECK_EQUAL(extractor::parseDuration("P1DT2H15M1S"), 94501);
    BOOST_CHECK_EQUAL(extractor::parseDuration("P4D"), 345600);
    BOOST_CHECK_EQUAL(extractor::parseDuration("PT4H"), 14400);
    BOOST_CHECK_EQUAL(extractor::parseDuration("PT71M"), 4260);
    BOOST_CHECK_EQUAL(extractor::parseDuration("PT022506"), 8706);
    BOOST_CHECK_EQUAL(extractor::parseDuration("PT02:25:06"), 8706);
    BOOST_CHECK_EQUAL(extractor::parseDuration("P3W"), 1814400);
}

BOOST_AUTO_TEST_CASE(iso_8601_durations_case_insensitive)
{
    BOOST_CHECK_EQUAL(extractor::parseDuration("PT15m"), 900);
    BOOST_CHECK_EQUAL(extractor::parseDuration("PT1h15m"), 4500);
    BOOST_CHECK_EQUAL(extractor::parseDuration("PT1h15m42s"), 4542);
    BOOST_CHECK_EQUAL(extractor::parseDuration("P2dT1h15m42s"), 177342);
}

BOOST_AUTO_TEST_SUITE_END()
