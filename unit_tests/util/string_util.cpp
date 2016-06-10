#include "util/string_util.hpp"

#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include <iostream>

BOOST_AUTO_TEST_SUITE(string_util)

using namespace osrm;
using namespace osrm::util;

BOOST_AUTO_TEST_CASE(json_escaping)
{
    std::string input{"\b\\"};
    std::string output{escape_JSON(input)};

    BOOST_CHECK_EQUAL(output, "\\b\\\\");

    input = "Aleja \"Solidarnosci\"";
    output = escape_JSON(input);
    BOOST_CHECK_EQUAL(output, "Aleja \\\"Solidarnosci\\\"");
}

BOOST_AUTO_TEST_CASE(print_int)
{
    const std::string input{"\b\\"};
    char buffer[12];
    buffer[11] = 0; // zero termination
    std::string output = printInt<11, 8>(buffer, 314158976);
    BOOST_CHECK_EQUAL(output, "3.14158976");

    buffer[11] = 0;
    output = printInt<11, 8>(buffer, 0);
    BOOST_CHECK_EQUAL(output, "0.00000000");

    output = printInt<11, 8>(buffer, -314158976);
    BOOST_CHECK_EQUAL(output, "-3.14158976");
}

BOOST_AUTO_TEST_SUITE_END()
