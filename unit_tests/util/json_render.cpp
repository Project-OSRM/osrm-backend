#include "util/json_container.hpp"
#include "util/json_renderer.hpp"

#include <boost/test/unit_test.hpp>

#include <iostream>

BOOST_AUTO_TEST_SUITE(json_renderer)

using namespace osrm;
using namespace osrm::util;

BOOST_AUTO_TEST_CASE(number_truncating)
{
    std::string str;
    json::Renderer<std::string> renderer(str);

    // this number would have more than 10 decimals if not truncated
    renderer(json::Number{42.9995999594999399299});
    BOOST_CHECK_EQUAL(str, "42.999599959");
}

BOOST_AUTO_TEST_CASE(integer)
{
    std::string str;
    json::Renderer<std::string> renderer(str);
    renderer(json::Number{42.0});
    BOOST_CHECK_EQUAL(str, "42");
}

BOOST_AUTO_TEST_SUITE_END()
