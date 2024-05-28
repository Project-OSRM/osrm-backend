#include "util/string_util.hpp"

#include <boost/test/unit_test.hpp>

#include <string>

BOOST_AUTO_TEST_SUITE(string_util)

using namespace osrm;
using namespace osrm::util;

BOOST_AUTO_TEST_CASE(json_escaping)
{
    std::string input{"\b\\"};
    std::string output;
    EscapeJSONString(input, output);

    BOOST_CHECK(RequiresJSONStringEscaping(input));
    BOOST_CHECK_EQUAL(output, "\\b\\\\");

    input = "Aleja \"Solidarnosci\"";
    output.clear();
    EscapeJSONString(input, output);
    BOOST_CHECK(RequiresJSONStringEscaping(input));
    BOOST_CHECK_EQUAL(output, "Aleja \\\"Solidarnosci\\\"");

    BOOST_CHECK(!RequiresJSONStringEscaping("Aleja Solidarnosci"));
}

BOOST_AUTO_TEST_SUITE_END()
