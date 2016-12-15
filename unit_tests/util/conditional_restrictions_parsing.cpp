#include "util/conditional_restrictions.hpp"

#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(conditional_restrictions)

BOOST_AUTO_TEST_CASE(check_conditional_restrictions_grammar)
{
    using osrm::util::ParseConditionalRestrictions;

    const std::string restrictions[] = {"120 @ (06:00-19:00)",
                                        "120 @ (06:00-20:00); 100 @ (22:00-06:00)",
                                        "120 @ 06:00-20:00; 100 @ 22:00-06:00",
                                        "destination @ (weight>5.5)",
                                        "no_left_turn @ (10:00-18:00 AND length>5)",
                                        "no @ (Mo-Fr 06:00-19:00; Sa 12:00-17:00)"};

    for (auto &restriction : restrictions)
    {
        BOOST_CHECK_MESSAGE(!ParseConditionalRestrictions(restriction).empty(),
                            "parsing " << restriction << " failed");
    }
}

BOOST_AUTO_TEST_CASE(check_conditional_restrictions_values)
{
    using osrm::util::ParseConditionalRestrictions;

    auto restrictions =
        ParseConditionalRestrictions("120 @ 06:00-20:00; 100 @ (22:00-06:00 AND length>5)");

    BOOST_REQUIRE_EQUAL(restrictions.size(), 2);
    BOOST_CHECK_EQUAL(restrictions.at(0).value, "120");
    BOOST_CHECK_EQUAL(restrictions.at(0).condition, "06:00-20:00");
    BOOST_CHECK_EQUAL(restrictions.at(1).value, "100");
    BOOST_CHECK_EQUAL(restrictions.at(1).condition, "22:00-06:00 AND length>5");
}

BOOST_AUTO_TEST_SUITE_END()
