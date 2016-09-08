#include <boost/optional/optional_io.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include "args.hpp"

#include "osrm/nearest_parameters.hpp"
#include "osrm/match_parameters.hpp"
#include "osrm/route_parameters.hpp"
#include "osrm/table_parameters.hpp"
#include "osrm/trip_parameters.hpp"

#include "osrm/coordinate.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/json_container.hpp"
#include "osrm/osrm.hpp"
#include "osrm/status.hpp"

BOOST_AUTO_TEST_SUITE(limits)

BOOST_AUTO_TEST_CASE(test_trip_limits)
{
    const auto args = get_args();
    BOOST_REQUIRE_EQUAL(args.size(), 1);

    using namespace osrm;

    EngineConfig config;
    config.storage_config = {args[0]};
    config.use_shared_memory = false;
    config.max_locations_trip = 2;

    OSRM osrm{config};

    TripParameters params;
    params.coordinates.emplace_back(util::FloatLongitude{}, util::FloatLatitude{});
    params.coordinates.emplace_back(util::FloatLongitude{}, util::FloatLatitude{});
    params.coordinates.emplace_back(util::FloatLongitude{}, util::FloatLatitude{});

    json::Object result;

    const auto rc = osrm.Trip(params, result);

    BOOST_CHECK(rc == Status::Error);

    // Make sure we're not accidentally hitting a guard code path before
    const auto code = result.values["code"].get<json::String>().value;
    BOOST_CHECK(code == "TooBig"); // per the New-Server API spec
}

BOOST_AUTO_TEST_CASE(test_route_limits)
{
    const auto args = get_args();
    BOOST_REQUIRE_EQUAL(args.size(), 1);

    using namespace osrm;

    EngineConfig config;
    config.storage_config = {args[0]};
    config.use_shared_memory = false;
    config.max_locations_viaroute = 2;

    OSRM osrm{config};

    RouteParameters params;
    params.coordinates.emplace_back(util::FloatLongitude{}, util::FloatLatitude{});
    params.coordinates.emplace_back(util::FloatLongitude{}, util::FloatLatitude{});
    params.coordinates.emplace_back(util::FloatLongitude{}, util::FloatLatitude{});

    json::Object result;

    const auto rc = osrm.Route(params, result);

    BOOST_CHECK(rc == Status::Error);

    // Make sure we're not accidentally hitting a guard code path before
    const auto code = result.values["code"].get<json::String>().value;
    BOOST_CHECK(code == "TooBig"); // per the New-Server API spec
}

BOOST_AUTO_TEST_CASE(test_table_limits)
{
    const auto args = get_args();
    BOOST_REQUIRE_EQUAL(args.size(), 1);

    using namespace osrm;

    EngineConfig config;
    config.storage_config = {args[0]};
    config.use_shared_memory = false;
    config.max_locations_distance_table = 2;

    OSRM osrm{config};

    TableParameters params;
    params.coordinates.emplace_back(util::FloatLongitude{}, util::FloatLatitude{});
    params.coordinates.emplace_back(util::FloatLongitude{}, util::FloatLatitude{});
    params.coordinates.emplace_back(util::FloatLongitude{}, util::FloatLatitude{});

    json::Object result;

    const auto rc = osrm.Table(params, result);

    BOOST_CHECK(rc == Status::Error);

    // Make sure we're not accidentally hitting a guard code path before
    const auto code = result.values["code"].get<json::String>().value;
    BOOST_CHECK(code == "TooBig"); // per the New-Server API spec
}

BOOST_AUTO_TEST_CASE(test_match_limits)
{
    const auto args = get_args();
    BOOST_REQUIRE_EQUAL(args.size(), 1);

    using namespace osrm;

    EngineConfig config;
    config.storage_config = {args[0]};
    config.use_shared_memory = false;
    config.max_locations_map_matching = 2;

    OSRM osrm{config};

    MatchParameters params;
    params.coordinates.emplace_back(util::FloatLongitude{}, util::FloatLatitude{});
    params.coordinates.emplace_back(util::FloatLongitude{}, util::FloatLatitude{});
    params.coordinates.emplace_back(util::FloatLongitude{}, util::FloatLatitude{});

    json::Object result;

    const auto rc = osrm.Match(params, result);

    BOOST_CHECK(rc == Status::Error);

    // Make sure we're not accidentally hitting a guard code path before
    const auto code = result.values["code"].get<json::String>().value;
    BOOST_CHECK(code == "TooBig"); // per the New-Server API spec
}

BOOST_AUTO_TEST_CASE(test_nearest_limits)
{
    using namespace osrm;

    const auto args = get_args();
    BOOST_REQUIRE_EQUAL(args.size(), 1);

    EngineConfig config;
    config.max_results_nearest = 1;
    config.use_shared_memory = false;
    OSRM osrm{config};

    NearestParameters params;
    params.number_of_results = 10;
    params.coordinates.emplace_back(util::FloatLongitude{}, util::FloatLatitude{});

    json::Object result;
    const auto rc = osrm.Nearest(params, result);
    BOOST_REQUIRE(rc == Status::Error);

    const auto code = result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "TooBig");

    // Also test the radius limit
    config.max_results_nearest = -1;
    config.max_radius_when_bearings = 10;
    osrm = OSRM{config};

    params.radiuses.push_back(20);
    params.bearings.push_back(boost::optional<engine::Bearing>{engine::Bearing{200, 10}});

    json::Object result2;
    const auto rc2 = osrm.Nearest(params, result2);
    BOOST_REQUIRE(rc2 == Status::Error);
    const auto code2 = result2.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code2, "TooBig");

}



BOOST_AUTO_TEST_SUITE_END()
