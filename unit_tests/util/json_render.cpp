#include "util/json_container.hpp"
#include "util/json_renderer.hpp"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(json_renderer)

using namespace osrm::util::json;

BOOST_AUTO_TEST_CASE(number_truncating)
{
    std::string str;
    Renderer<std::string> renderer(str);

    // this number would have more than 12 decimals if not truncated
    renderer(Number{42.9995999594999399299});
    BOOST_CHECK_EQUAL(str, "42.9995999595");
}

BOOST_AUTO_TEST_CASE(integer)
{
    std::string str;
    Renderer<std::string> renderer(str);
    renderer(Number{42.0});
    BOOST_CHECK_EQUAL(str, "42");
}

BOOST_AUTO_TEST_CASE(test_json_issue_6531)
{
    std::string output;
    osrm::util::json::Renderer<std::string> renderer(output);
    renderer(0.0000000000017114087924596788);
    BOOST_CHECK_EQUAL(output, "1.71140879246e-12");

    output.clear();
    renderer(42.0);
    BOOST_CHECK_EQUAL(output, "42");

    output.clear();
    renderer(42.1);
    BOOST_CHECK_EQUAL(output, "42.1");

    output.clear();
    renderer(42.12);
    BOOST_CHECK_EQUAL(output, "42.12");

    output.clear();
    renderer(42.123);
    BOOST_CHECK_EQUAL(output, "42.123");

    output.clear();
    renderer(42.1234);
    BOOST_CHECK_EQUAL(output, "42.1234");

    output.clear();
    renderer(42.12345);
    BOOST_CHECK_EQUAL(output, "42.12345");

    output.clear();
    renderer(42.123456);
    BOOST_CHECK_EQUAL(output, "42.123456");

    output.clear();
    renderer(42.1234567);
    BOOST_CHECK_EQUAL(output, "42.1234567");

    output.clear();
    renderer(42.12345678);
    BOOST_CHECK_EQUAL(output, "42.12345678");

    output.clear();
    renderer(42.123456789);
    BOOST_CHECK_EQUAL(output, "42.123456789");

    output.clear();
    renderer(0.12345678912345);
    BOOST_CHECK_EQUAL(output, "0.123456789123");

    output.clear();
    renderer(0.123456789);
    BOOST_CHECK_EQUAL(output, "0.123456789");

    output.clear();
    renderer(0.12345678916);
    BOOST_CHECK_EQUAL(output, "0.12345678916");

    output.clear();
    renderer(12345678912345678.0);
    BOOST_CHECK_EQUAL(output, "1.23456789123e+16");

    // handle large osm ids, till 12 digits
    output.clear();
    renderer(100396615812);
    BOOST_CHECK_EQUAL(output, "100396615812");
}

BOOST_AUTO_TEST_SUITE_END()
