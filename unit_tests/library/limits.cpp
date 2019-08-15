#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include "osrm/match_parameters.hpp"
#include "osrm/nearest_parameters.hpp"
#include "osrm/route_parameters.hpp"
#include "osrm/table_parameters.hpp"
#include "osrm/trip_parameters.hpp"

#include "osrm/coordinate.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/json_container.hpp"
#include "osrm/osrm.hpp"
#include "osrm/status.hpp"

namespace
{
osrm::util::Coordinate getZeroCoordinate()
{
    return {osrm::util::FloatLongitude{0}, osrm::util::FloatLatitude{0}};
}
}

BOOST_AUTO_TEST_SUITE(limits)

BOOST_AUTO_TEST_CASE(test_trip_limits)
{
    using namespace osrm;

    EngineConfig config;
    config.storage_config = {OSRM_TEST_DATA_DIR "/ch/monaco.osrm"};
    config.use_shared_memory = false;
    config.max_locations_trip = 2;

    OSRM osrm{config};

    TripParameters params;
    params.coordinates.emplace_back(getZeroCoordinate());
    params.coordinates.emplace_back(getZeroCoordinate());
    params.coordinates.emplace_back(getZeroCoordinate());

    engine::api::ResultT result = json::Object();

    const auto rc = osrm.Trip(params, result);

    BOOST_CHECK(rc == Status::Error);

    // Make sure we're not accidentally hitting a guard code path before
    auto &json_result = result.get<json::Object>();
    const auto code = json_result.values["code"].get<json::String>().value;
    BOOST_CHECK(code == "TooBig"); // per the New-Server API spec
}

BOOST_AUTO_TEST_CASE(test_route_limits)
{
    using namespace osrm;

    EngineConfig config;
    config.storage_config = {OSRM_TEST_DATA_DIR "/ch/monaco.osrm"};
    config.use_shared_memory = false;
    config.max_locations_viaroute = 2;

    OSRM osrm{config};

    RouteParameters params;
    params.coordinates.emplace_back(getZeroCoordinate());
    params.coordinates.emplace_back(getZeroCoordinate());
    params.coordinates.emplace_back(getZeroCoordinate());

    engine::api::ResultT result = json::Object();

    const auto rc = osrm.Route(params, result);

    BOOST_CHECK(rc == Status::Error);

    // Make sure we're not accidentally hitting a guard code path before
    auto &json_result = result.get<json::Object>();
    const auto code = json_result.values["code"].get<json::String>().value;
    BOOST_CHECK(code == "TooBig"); // per the New-Server API spec
}

BOOST_AUTO_TEST_CASE(test_table_limits)
{
    using namespace osrm;

    EngineConfig config;
    config.storage_config = {OSRM_TEST_DATA_DIR "/ch/monaco.osrm"};
    config.use_shared_memory = false;
    config.max_locations_distance_table = 2;

    OSRM osrm{config};

    TableParameters params;
    params.coordinates.emplace_back(getZeroCoordinate());
    params.coordinates.emplace_back(getZeroCoordinate());
    params.coordinates.emplace_back(getZeroCoordinate());

    engine::api::ResultT result = json::Object();

    const auto rc = osrm.Table(params, result);

    BOOST_CHECK(rc == Status::Error);

    // Make sure we're not accidentally hitting a guard code path before
    auto &json_result = result.get<json::Object>();
    const auto code = json_result.values["code"].get<json::String>().value;
    BOOST_CHECK(code == "TooBig"); // per the New-Server API spec
}

BOOST_AUTO_TEST_CASE(test_match_coordinate_limits)
{
    using namespace osrm;

    EngineConfig config;
    config.storage_config = {OSRM_TEST_DATA_DIR "/ch/monaco.osrm"};
    config.use_shared_memory = false;
    config.max_locations_map_matching = 2;

    OSRM osrm{config};

    MatchParameters params;
    params.coordinates.emplace_back(getZeroCoordinate());
    params.coordinates.emplace_back(getZeroCoordinate());
    params.coordinates.emplace_back(getZeroCoordinate());

    engine::api::ResultT result = json::Object();

    const auto rc = osrm.Match(params, result);

    BOOST_CHECK(rc == Status::Error);

    // Make sure we're not accidentally hitting a guard code path before
    auto &json_result = result.get<json::Object>();
    const auto code = json_result.values["code"].get<json::String>().value;
    BOOST_CHECK(code == "TooBig"); // per the New-Server API spec
}

BOOST_AUTO_TEST_CASE(test_match_radiuses_limits)
{
    using namespace osrm;

    EngineConfig config;
    config.storage_config = {OSRM_TEST_DATA_DIR "/ch/monaco.osrm"};
    config.use_shared_memory = false;
    config.max_radius_map_matching = 2.0;

    OSRM osrm{config};

    MatchParameters params;
    osrm::util::Coordinate coord1 = {osrm::util::FloatLongitude{7.41748809814453},
                                     osrm::util::FloatLatitude{43.73558473009846}};
    osrm::util::Coordinate coord2 = {osrm::util::FloatLongitude{7.417193055152893},
                                     osrm::util::FloatLatitude{43.735162245104775}};
    params.coordinates.emplace_back(coord1);
    params.coordinates.emplace_back(coord2);
    params.radiuses.emplace_back(3.0);
    params.radiuses.emplace_back(2.0);

    engine::api::ResultT result = json::Object();

    const auto rc = osrm.Match(params, result);

    BOOST_CHECK(rc == Status::Error);

    // Make sure we're not accidentally hitting a guard code path before
    auto &json_result = result.get<json::Object>();
    const auto code = json_result.values["code"].get<json::String>().value;
    BOOST_CHECK(code == "TooBig"); // per the New-Server API spec
}

BOOST_AUTO_TEST_CASE(test_nearest_limits)
{
    using namespace osrm;

    EngineConfig config;
    config.storage_config = {OSRM_TEST_DATA_DIR "/ch/monaco.osrm"};
    config.use_shared_memory = false;
    config.max_results_nearest = 2;

    OSRM osrm{config};

    NearestParameters params;
    params.coordinates.emplace_back(getZeroCoordinate());
    params.number_of_results = 10000;

    engine::api::ResultT result = json::Object();

    const auto rc = osrm.Nearest(params, result);

    BOOST_CHECK(rc == Status::Error);

    // Make sure we're not accidentally hitting a guard code path before
    auto &json_result = result.get<json::Object>();
    const auto code = json_result.values["code"].get<json::String>().value;
    BOOST_CHECK(code == "TooBig"); // per the New-Server API spec
}

BOOST_AUTO_TEST_SUITE_END()
