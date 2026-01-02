#include "util/json_container.hpp"
#include "util/json_renderer.hpp"

#include <boost/test/unit_test.hpp>
#include <limits>

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

BOOST_AUTO_TEST_CASE(test_json_issue_6531)
{
    std::string output;
    osrm::util::json::Renderer<std::string> renderer(output);
    renderer(0.0000000000017114087924596788);
    BOOST_CHECK_EQUAL(output, "1.711408792e-12");

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
    BOOST_CHECK_EQUAL(output, "42.12345679");

    output.clear();
    renderer(0.12345678912345);
    BOOST_CHECK_EQUAL(output, "0.1234567891");

    output.clear();
    renderer(0.123456789);
    BOOST_CHECK_EQUAL(output, "0.123456789");

    output.clear();
    renderer(0.12345678916);
    BOOST_CHECK_EQUAL(output, "0.1234567892");

    output.clear();
    renderer(123456789123456789.);
    BOOST_CHECK_EQUAL(output, "1.234567891e+17");
}

// Tests for issue #7016: large OSM IDs rendered as scientific notation
BOOST_AUTO_TEST_CASE(test_large_osm_ids)
{
    std::string output;
    Renderer<std::string> renderer(output);

    // Test from original issue #7016 - was showing as 1.111742119e+10
    output.clear();
    renderer(Number{11117421192.0});
    BOOST_CHECK_EQUAL(output, "11117421192");

    // Test precision loss case - was becoming 10003966160
    output.clear();
    renderer(Number{10003966158.0});
    BOOST_CHECK_EQUAL(output, "10003966158");

    // Test 11-digit OSM IDs (current max range)
    output.clear();
    renderer(Number{12345678901.0});
    BOOST_CHECK_EQUAL(output, "12345678901");

    // Test 12-digit values
    output.clear();
    renderer(Number{100396615812.0});
    BOOST_CHECK_EQUAL(output, "100396615812");

    // Test values up to 2^53 (max exact integer in double)
    output.clear();
    renderer(Number{9007199254740992.0}); // 2^53
    BOOST_CHECK_EQUAL(output, "9007199254740992");

    // Test zero
    output.clear();
    renderer(Number{0.0});
    BOOST_CHECK_EQUAL(output, "0");

    // Test small integers
    output.clear();
    renderer(Number{42.0});
    BOOST_CHECK_EQUAL(output, "42");

    // Test 1
    output.clear();
    renderer(Number{1.0});
    BOOST_CHECK_EQUAL(output, "1");
}

BOOST_AUTO_TEST_CASE(test_floats_unchanged)
{
    std::string output;
    Renderer<std::string> renderer(output);

    // Floats should still use {:.10g} format
    output.clear();
    renderer(Number{3.14159});
    BOOST_CHECK_EQUAL(output, "3.14159");

    output.clear();
    renderer(Number{42.5});
    BOOST_CHECK_EQUAL(output, "42.5");

    // Very small numbers still use scientific notation
    output.clear();
    renderer(Number{0.0000000000017114087924596788});
    BOOST_CHECK_EQUAL(output, "1.711408792e-12");

    // Negative numbers use float format
    output.clear();
    renderer(Number{-42.0});
    BOOST_CHECK_EQUAL(output, "-42");

    output.clear();
    renderer(Number{42.9995999594999399299});
    BOOST_CHECK_EQUAL(output, "42.99959996");
}

BOOST_AUTO_TEST_CASE(test_edge_cases)
{
    std::string output;
    Renderer<std::string> renderer(output);

    // Negative zero should be treated as a whole number: it passes the non-negativity
    // check, std::trunc(-0.0) == -0.0, so it goes through the integer path and is "0"
    output.clear();
    renderer(Number{-0.0});
    BOOST_CHECK_EQUAL(output, "0");

    // Very large float (larger than max uint64) uses scientific notation
    output.clear();
    renderer(Number{1.8446744073709552e+19}); // > uint64 max
    BOOST_CHECK_EQUAL(output, "1.844674407e+19");
}

BOOST_AUTO_TEST_CASE(test_nan_infinity_produce_null)
{
    std::string output;
    Renderer<std::string> renderer(output);

    // NaN should produce null (valid JSON)
    output.clear();
    renderer(Number{std::numeric_limits<double>::quiet_NaN()});
    BOOST_CHECK_EQUAL(output, "null");

    // Positive infinity should produce null
    output.clear();
    renderer(Number{std::numeric_limits<double>::infinity()});
    BOOST_CHECK_EQUAL(output, "null");

    // Negative infinity should produce null
    output.clear();
    renderer(Number{-std::numeric_limits<double>::infinity()});
    BOOST_CHECK_EQUAL(output, "null");
}

BOOST_AUTO_TEST_CASE(test_nan_in_array)
{
    std::string output;
    Renderer<std::string> renderer(output);

    Array arr;
    arr.values.push_back(Number{42.0});
    arr.values.push_back(Number{std::numeric_limits<double>::quiet_NaN()});
    arr.values.push_back(Number{3.14});
    renderer(arr);
    BOOST_CHECK_EQUAL(output, "[42,null,3.14]");
}

BOOST_AUTO_TEST_CASE(test_integer_in_array)
{
    std::string output;
    Renderer<std::string> renderer(output);

    Array arr;
    arr.values.push_back(Number{11117421192.0});
    arr.values.push_back(Number{10003966158.0});
    arr.values.push_back(Number{3.14});
    renderer(arr);
    BOOST_CHECK_EQUAL(output, "[11117421192,10003966158,3.14]");
}

BOOST_AUTO_TEST_SUITE_END()
