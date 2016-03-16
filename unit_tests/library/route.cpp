#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>

#include "args.hpp"
#include "fixture.hpp"

#include "osrm/route_parameters.hpp"

#include "osrm/coordinate.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/json_container.hpp"
#include "osrm/status.hpp"
#include "osrm/osrm.hpp"

BOOST_AUTO_TEST_SUITE(route)

BOOST_AUTO_TEST_CASE(test_route)
{
    const auto args = get_args();
    auto osrm = get_osrm(args.at(0));

    using namespace osrm;

    RouteParameters params;

    params.coordinates.emplace_back(util::FloatLongitude{}, util::FloatLatitude{});
    params.coordinates.emplace_back(util::FloatLongitude{}, util::FloatLatitude{});

    json::Object result;

    const auto rc = osrm.Route(params, result);

    BOOST_CHECK(rc == Status::Ok || rc == Status::Error);
}

BOOST_AUTO_TEST_SUITE_END()
