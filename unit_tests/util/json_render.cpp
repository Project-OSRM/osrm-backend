#include "util/json_container.hpp"
#include "util/json_renderer.hpp"

#include <boost/test/unit_test.hpp>

#include <iostream>

BOOST_AUTO_TEST_SUITE(json_renderer)

using namespace osrm::util::json;

BOOST_AUTO_TEST_CASE(number_truncating)
{
    std::string str;
    Renderer<std::string> renderer(str);

    // this number would have more than 10 decimals if not truncated
    renderer(Number{42.9995999594999399299});
    BOOST_CHECK_EQUAL(str, "42.99959996");
}

BOOST_AUTO_TEST_CASE(integer)
{
    std::string str;
    Renderer<std::string> renderer(str);
    renderer(Number{42.0});
    BOOST_CHECK_EQUAL(str, "42");
}

BOOST_AUTO_TEST_CASE(test_json_issue_6531) {
    std::string output;
    osrm::util::json::Renderer<std::string> renderer(output);
    renderer(0.0000000000017114087924596788);
    BOOST_CHECK_EQUAL(output, "1.711408792e-12");
}

BOOST_AUTO_TEST_SUITE_END()
