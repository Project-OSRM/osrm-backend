#include "server/header_size.hpp"

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <string>

std::string generateWorstCaseRequest(unsigned num_coordinates)
{
    if (num_coordinates == 0)
        return "/table/v1/driving/";

    std::string request;
    request.reserve(50 * num_coordinates);
    request += "/table/v1/driving/";

    // 1. Coordinates: (2-3 (major) + 1 (dot) + 6 (decimals)) * 2 = 20 chars per coordinate
    for (unsigned i = 0; i < num_coordinates; ++i)
    {
        request += "179.123456,89.123456";
        if (i < num_coordinates - 1)
            request += ";";
    }

    request += "?";

    // 2. Radiuses: 3 digits
    request += "radiuses=";
    for (unsigned i = 0; i < num_coordinates; ++i)
    {
        request += "999";
        if (i < num_coordinates - 1)
            request += ";";
    }
    request += "&";

    // 3. Sources and Destinations: 4 digits
    std::string sources_str = "sources=";
    std::string destinations_str = "destinations=";
    std::string index_str = "";
    for (unsigned i = 0; i < num_coordinates; ++i)
    {
        index_str += "9999";
        if (i < num_coordinates - 1)
        {
            index_str += ";";
        }
    }
    request += sources_str + index_str + "&" + destinations_str + index_str + "&";

    // 4. Approaches: "unrestricted" has 12 chars
    request += "approaches=";
    for (unsigned i = 0; i < num_coordinates; ++i)
    {
        request += "unrestricted";
        if (i < num_coordinates - 1)
            request += ";";
    }

    return request;
}

BOOST_AUTO_TEST_SUITE(header_size)

using namespace osrm;
using namespace osrm::server;
using namespace osrm::engine;

BOOST_AUTO_TEST_CASE(minimum_header_size)
{
    EngineConfig config;
    config.max_locations_trip = 0;
    config.max_locations_viaroute = 0;
    config.max_locations_distance_table = 0;
    config.max_locations_map_matching = 0;

    unsigned result = deriveMaxHeaderSize(config);
    BOOST_CHECK_EQUAL(result, 8 * 1024);
}

BOOST_AUTO_TEST_CASE(negative_values_treated_as_zero)
{
    // Test with default EngineConfig values (all -1 for default)
    EngineConfig config;

    unsigned result = deriveMaxHeaderSize(config);
    BOOST_CHECK_EQUAL(result, 8 * 1024);
}

BOOST_AUTO_TEST_CASE(calculation_formula_verification)
{
    auto run_test_for_n_coordinates = [](unsigned num_coords)
    {
        EngineConfig config;
        config.max_locations_trip = num_coords;
        config.max_locations_viaroute = num_coords;
        config.max_locations_distance_table = num_coords;
        config.max_locations_map_matching = num_coords;

        unsigned calculated_size = deriveMaxHeaderSize(config);
        std::string worst_case_request = generateWorstCaseRequest(num_coords);

        BOOST_CHECK_GT(calculated_size, worst_case_request.length());
    };

    run_test_for_n_coordinates(10);
    run_test_for_n_coordinates(100);
    run_test_for_n_coordinates(1000);
    run_test_for_n_coordinates(10000);
}

BOOST_AUTO_TEST_SUITE_END()
